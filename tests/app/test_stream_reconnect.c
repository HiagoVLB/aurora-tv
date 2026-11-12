/* Reconnect is offered only after an error ended a live stream; the target keeps
 * host/app identity and a new session start clears it. */
#include "stream/session.h"
#include "stream/session_priv.h"
#include "errors.h"
#include <SDL.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static session_t *make_session(STREAMING_STATE state, SERVER_DATA *server) {
    static session_t session;
    memset(&session, 0, sizeof(session));
    session.mutex = SDL_CreateMutex();
    session.cond = SDL_CreateCond();
    session.state = state;
    session.server = server;
    session.app_id = 374846262;
    strcpy(session.app_uuid, "12345678-1234-1234-1234-123456789abc");
    session.app_name = "Desktop";
    session.input.session = &session;
    session.config.view_only = true; /* no evmouse in this fixture */
    return &session;
}

int main(void) {
    SDL_Init(0);
    SERVER_DATA server = {.uuid = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee"};

    /* Network drop while video was showing: Reconnect offered, message set. */
    session_t *s = make_session(STREAMING_STREAMING, &server);
    streaming_error(s, GS_OK, "");
    session_interrupt(s, false, STREAMING_INTERRUPT_NETWORK);
    assert(streaming_errno == STREAMING_INTERRUPT_NETWORK);
    assert(strcmp(streaming_errmsg, "The connection to the PC was lost.") == 0);
    assert(streaming_reconnect_target.valid);
    assert(strcmp(streaming_reconnect_target.host_uuid, server.uuid) == 0);
    assert(streaming_reconnect_target.app_id == 374846262);
    assert(strcmp(streaming_reconnect_target.app_uuid, "12345678-1234-1234-1234-123456789abc") == 0);
    assert(strcmp(streaming_reconnect_target.app_name, "Desktop") == 0);

    /* Next session start clears it. */
    s = make_session(STREAMING_CONNECTING, &server);
    streaming_error(s, GS_OK, "");
    assert(!streaming_reconnect_target.valid);

    /* Failure before video (launch/connect): no Reconnect. */
    session_interrupt(s, false, STREAMING_INTERRUPT_NETWORK);
    assert(streaming_errno == STREAMING_INTERRUPT_NETWORK && !streaming_reconnect_target.valid);

    /* User disconnect, Home (background) and host-ended streams are not errors. */
    static const streaming_interrupt_reason_t not_errors[] = {
            STREAMING_INTERRUPT_USER, STREAMING_INTERRUPT_BACKGROUND, STREAMING_INTERRUPT_HOST};
    for (size_t i = 0; i < sizeof(not_errors) / sizeof(not_errors[0]); i++) {
        s = make_session(STREAMING_STREAMING, &server);
        streaming_error(s, GS_OK, "");
        session_interrupt(s, false, not_errors[i]);
        assert(streaming_errno == GS_OK && !streaming_reconnect_target.valid);
    }

    /* Watchdog and decoder errors on a live stream also offer Reconnect. */
    s = make_session(STREAMING_STREAMING, &server);
    streaming_error(s, GS_OK, "");
    session_interrupt(s, false, STREAMING_INTERRUPT_WATCHDOG);
    assert(streaming_reconnect_target.valid);
    assert(strcmp(streaming_errmsg, "The video from the PC stopped arriving.") == 0);
    s = make_session(STREAMING_STREAMING, &server);
    streaming_error(s, GS_OK, "");
    session_interrupt(s, false, STREAMING_INTERRUPT_DECODER);
    assert(streaming_reconnect_target.valid);

    /* Only the first interrupt counts. */
    s = make_session(STREAMING_STREAMING, &server);
    streaming_error(s, GS_OK, "");
    session_interrupt(s, false, STREAMING_INTERRUPT_USER);
    session_interrupt(s, false, STREAMING_INTERRUPT_NETWORK);
    assert(!streaming_reconnect_target.valid && streaming_errno == GS_OK);

    puts("Stream reconnect offer: live errors only, cleared on next start passed");
    return 0;
}
