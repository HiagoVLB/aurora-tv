/* Replace discovery's server during /cancel; the request must own its snapshot. */
#define app_gs_client_new test_client_new
#define gs_quit_app test_quit
#define gs_destroy test_client_destroy
#define gs_get_error test_get_error
#define pcmanager_lock test_lock
#define pcmanager_unlock test_unlock
#define pclist_find_by_uuid test_find
#define pclist_upsert test_upsert
#define serverdata_clone test_clone
#include "../../../../src/app/backend/pcmanager/worker/quit_app.c"
#undef serverdata_clone
PSERVER_DATA serverdata_clone(const SERVER_DATA *src);
#include <assert.h>
#include <stdio.h>

static pclist_t node;
static bool present;
static int depth, result, clients, destroyed, updates;
static PSERVER_DATA request;

void test_lock(pcmanager_t *m) { (void)m; depth++; }
void test_unlock(pcmanager_t *m) { (void)m; assert(depth > 0); depth--; }
pclist_t *test_find(pcmanager_t *m, const uuidstr_t *uuid) {
    (void)m; (void)uuid;
    assert(depth > 0);
    return present ? &node : NULL;
}
PSERVER_DATA test_clone(const SERVER_DATA *src) {
    assert(depth > 0);
    return serverdata_clone(src);
}
GS_CLIENT test_client_new(app_t *app) { clients++; return (GS_CLIENT)app; }
void test_client_destroy(GS_CLIENT c) { assert(c); destroyed++; }
int test_get_error(const char **error) { *error = "cancel failed"; return GS_ERROR; }
int test_quit(GS_CLIENT c, PSERVER_DATA server) {
    assert(c && depth == 0 && server != node.server);
    request = server;
    assert(!strcmp(server->hostname, "original"));
    /* The real discovery path replaces and frees node->server during I/O. */
    serverdata_free(node.server);
    node.server = serverdata_new();
    node.server->hostname = strdup("discovered");
    assert(!strcmp(server->hostname, "original"));
    server->currentGame = 0;
    return result;
}
void test_upsert(pcmanager_t *m, const uuidstr_t *uuid, const SERVER_STATE *state, SERVER_DATA *server) {
    (void)m; (void)uuid;
    assert(depth == 0 && server == request && state->code == SERVER_STATE_AVAILABLE);
    serverdata_free(node.server);
    node.server = server;
    updates++;
}
int main(void) {
    app_t app = {0};
    pcmanager_t manager = {0};
    worker_context_t ctx = {.app = &app, .manager = &manager};
    assert(worker_quit_app(&ctx) == GS_ERROR);
    assert(depth == 0 && clients == 0);
    present = true;
    for (int i = 0; i < 2; i++) {
        node.server = serverdata_new();
        node.server->hostname = strdup("original");
        result = i ? GS_ERROR : GS_OK;
        assert(worker_quit_app(&ctx) == result);
        assert(depth == 0 && clients == i + 1 && destroyed == clients && updates == 1);
        if (i) {
            assert(!strcmp(ctx.error, "cancel failed"));
            assert(!strcmp(node.server->hostname, "discovered"));
            free(ctx.error);
        } else {
            assert(node.server == request && node.server->currentGame == 0);
        }
        serverdata_free(node.server);
    }
    puts("End session: locked snapshot, discovery replacement during I/O, success/error ownership and client cleanup passed");
    return 0;
}
