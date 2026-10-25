/* Receive callbacks must queue notices; only the main thread may translate/show them. */
#define app_bus_post test_post
#define streaming_notice_show test_notice
#define locstr test_locstr
#define connection_callbacks test_connection_callbacks
#define session_connection_callbacks test_session_connection_callbacks
#include "../../src/app/stream/connection/session_connnection.c"
#include <assert.h>
#include <stdio.h>

static SDL_threadID main_thread;
static bus_actionfunc queued_action;
static void *queued_data;
static unsigned posts, shown, translations;
static const char *message;

bool test_post(app_t *app, bus_actionfunc action, void *data) {
    assert(app && SDL_ThreadID() != main_thread);
    assert(!queued_action);
    posts++;
    queued_action = action;
    queued_data = data;
    return true;
}
void test_notice(const char *text) {
    assert(SDL_ThreadID() == main_thread);
    shown++;
    message = text;
}
const char *test_locstr(const char *text) {
    assert(SDL_ThreadID() == main_thread);
    translations++;
    return text;
}
static int receive_status(void *data) {
    connection_status_update((int)(intptr_t)data);
    return 0;
}
static void receive(int status) {
    SDL_Thread *t = SDL_CreateThread(receive_status, "notice-receive", (void *)(intptr_t)status);
    assert(t);
    SDL_WaitThread(t, NULL);
}
static void dispatch(void) {
    assert(queued_action);
    queued_action(queued_data);
    queued_action = NULL;
}
int main(void) {
    assert(SDL_Init(0) == 0);
    main_thread = SDL_ThreadID();
    app_t app = {0};
    session_t session = {.app = &app};
    receive(CONN_STATUS_POOR); /* No attached session. */
    assert(posts == 0 && shown == 0);
    current_session = &session;
    receive(CONN_STATUS_POOR);
    assert(posts == 1 && shown == 0 && translations == 0);
    dispatch();
    assert(shown == 1 && translations == 1 && !strcmp(message, "Unstable connection."));
    receive(CONN_STATUS_OKAY);
    assert(posts == 2 && shown == 1);
    dispatch();
    assert(shown == 2 && translations == 1 && message == NULL);
    receive(-99);
    assert(posts == 2 && !queued_action);
    current_session = NULL;
    receive(CONN_STATUS_OKAY);
    assert(posts == 2);
    SDL_Quit();
    puts("Connection notices: queued by receive thread, translated/displayed by main thread, idle/unknown ignored");
    return 0;
}
