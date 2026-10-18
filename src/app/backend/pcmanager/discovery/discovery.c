#include "discovery.h"
#include "throttle.h"

#include "impl/impl.h"

#include "util/bus.h"
#include "logging.h"

static int discovery_worker_wrapper(void *arg);

static void task_destroy(discovery_task_t *task) {
    SDL_WaitThread(task->thread, NULL);
    SDL_DestroyMutex(task->lock);
    SDL_free(task);
}

/* Called with discovery->lock held. Completed workers no longer call into SDL's
 * event queue, so reaping them cannot wait for an event-dispatch callback. */
static void reap_completed(discovery_t *discovery) {
    discovery_task_t **link = &discovery->tasks;
    while (*link) {
        discovery_task_t *task = *link;
        if (!SDL_AtomicGet(&task->completed)) {
            link = &task->next;
            continue;
        }
        *link = task->next;
        if (discovery->task == task) discovery->task = NULL;
        task_destroy(task);
    }
}

void discovery_init(discovery_t *discovery, discovery_callback callback, void *user_data) {
    discovery->lock = SDL_CreateMutex();
    discovery->task = NULL;
    discovery->tasks = NULL;
    discovery_throttle_init(&discovery->throttle, callback, user_data);
}

void discovery_deinit(discovery_t *discovery) {
    SDL_LockMutex(discovery->lock);
    discovery_task_t *tasks = discovery->tasks;
    discovery->task = NULL;
    discovery->tasks = NULL;
    for (discovery_task_t *task = tasks; task; task = task->next) discovery_worker_stop(task);
    SDL_UnlockMutex(discovery->lock);
    /* Only final destruction joins live workers. Normal stop stays asynchronous
     * because launcher transitions can run while SDL's event queue is locked. */
    while (tasks) {
        discovery_task_t *next = tasks->next;
        task_destroy(tasks);
        tasks = next;
    }
    discovery_throttle_deinit(&discovery->throttle);
    SDL_DestroyMutex(discovery->lock);
}

void discovery_start(discovery_t *discovery) {
    SDL_LockMutex(discovery->lock);
    reap_completed(discovery);
    if (discovery->task != NULL) {
        SDL_UnlockMutex(discovery->lock);
        return;
    }
    discovery_task_t *task = SDL_calloc(1, sizeof(discovery_task_t));
    if (!task) {
        SDL_UnlockMutex(discovery->lock);
        return;
    }
    task->discovery = discovery;
    task->lock = SDL_CreateMutex();
    if (!task->lock) {
        SDL_free(task);
        SDL_UnlockMutex(discovery->lock);
        return;
    }
    task->thread = SDL_CreateThread(discovery_worker_wrapper, "discovery", task);
    if (!task->thread) {
        SDL_DestroyMutex(task->lock);
        SDL_free(task);
        SDL_UnlockMutex(discovery->lock);
        return;
    }
    commons_log_info("Discovery", "Start task %p", task);
    task->next = discovery->tasks;
    discovery->tasks = task;
    discovery->task = task;
    SDL_UnlockMutex(discovery->lock);
}

void discovery_stop(discovery_t *discovery) {
    SDL_LockMutex(discovery->lock);
    discovery_task_t *task = discovery->task;
    if (task != NULL) {
        discovery->task = NULL;
        discovery_worker_stop(task);
    }
    SDL_UnlockMutex(discovery->lock);
}

void discovery_discovered(struct discovery_t *discovery, const sockaddr_t *addr) {
    discovery_throttle_on_discovered(&discovery->throttle, addr, 10000);
}

int discovery_worker_wrapper(void *arg) {
    discovery_task_t *task = arg;
    int result = discovery_worker(task);
    SDL_AtomicSet(&task->completed, 1);
    /* Owner reclaims the task and its mutex after joining this worker. */
    return result;
}
