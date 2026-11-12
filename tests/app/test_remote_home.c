/* Run the actual webOS remote mapping with firmware/UI entry points stubbed. */
#define app_webos_go_home test_go_home
#define streaming_soft_keyboard_shown test_keyboard_shown
#define ui_should_block_input test_block_input
#define bus_pushevent test_post_event
#include "platform/webos/sdl_webos_scancode.h"
#include "../../src/app/platform/webos/keyboard_webos.c"
#include <assert.h>
#include <stdio.h>

static int ribbon_count;
static session_t *home_session;
static int last_event, event_count;
void test_go_home(session_t *s) {
    ribbon_count++;
    home_session = s;
}
bool test_keyboard_shown(void) { return false; }
bool test_block_input(void) { return false; }
bool test_post_event(int which, void *data1, void *data2) {
    (void) data1;
    (void) data2;
    last_event = which;
    event_count++;
    return true;
}

int main(void) {
    session_t session = {0};
    stream_input_t input = {.session = &session};
    SDL_KeyboardEvent event = {.keysym.scancode = SDL_SCANCODE_WEBOS_HOME};
    short vk;
    /* Home is the TV's Home in every state: once, on release, never sent to the
     * PC; with a stream open it gets the session so it disconnects first. */
    for (int state = 0; state < 3; state++) {
        input.session = state == 2 ? NULL : &session;
        input.view_only = state == 1;
        ribbon_count = 0;
        event.state = SDL_PRESSED;
        vk = 0;
        assert(stream_input_webos_intercept_remote_keys(&input, &event, &vk));
        assert(vk == 0 && ribbon_count == 0);
        event.state = SDL_RELEASED;
        assert(stream_input_webos_intercept_remote_keys(&input, &event, &vk));
        assert(vk == 0 && ribbon_count == 1);
        assert(home_session == input.session);
    }
    /* Green is the Windows key: both edges reach the host. */
    input.session = &session;
    input.view_only = false;
    event.keysym.scancode = SDL_SCANCODE_WEBOS_GREEN;
    for (int down = 0; down <= 1; down++) {
        event.state = down ? SDL_PRESSED : SDL_RELEASED;
        vk = 0;
        assert(!stream_input_webos_intercept_remote_keys(&input, &event, &vk));
        assert(vk == VK_LWIN);
    }
    /* View-only and idle never send it. */
    for (int idle = 0; idle <= 1; idle++) {
        input.session = idle ? NULL : &session;
        input.view_only = !idle;
        event.state = SDL_PRESSED;
        vk = 0;
        assert(stream_input_webos_intercept_remote_keys(&input, &event, &vk));
        assert(vk == 0);
    }
    /* Physical keyboard Home is handled by the regular keyboard mapper. */
    input.session = &session;
    input.view_only = false;
    event.keysym.scancode = SDL_SCANCODE_HOME;
    vk = 0;
    assert(!stream_input_webos_intercept_remote_keys(&input, &event, &vk));
    assert(vk == 0);
    /* webOS delivers remote digits as SDL 0..9; preserve both key edges. */
    for (int digit = 0; digit <= 9; digit++) {
        event.keysym.scancode = digit == 0 ? SDL_SCANCODE_0 : SDL_SCANCODE_1 + digit - 1;
        for (int down = 0; down <= 1; down++) {
            event.state = down ? SDL_PRESSED : SDL_RELEASED;
            vk = 0;
            assert(!stream_input_webos_intercept_remote_keys(&input, &event, &vk));
            assert(vk == VK_NUMPAD0 + digit);
        }
    }
    /* Yellow opens the stream menu and Red asks to disconnect: once per press,
     * never sent to the PC. */
    static const struct { int scancode; int user_event; } colors[] = {
            {SDL_SCANCODE_WEBOS_YELLOW, USER_OPEN_OVERLAY},
            {SDL_SCANCODE_WEBOS_RED, USER_ASK_DISCONNECT},
    };
    for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
        event.keysym.scancode = colors[i].scancode;
        event_count = 0;
        last_event = 0;
        for (int down = 1; down >= 0; down--) {
            event.state = down ? SDL_PRESSED : SDL_RELEASED;
            vk = 0;
            assert(stream_input_webos_intercept_remote_keys(&input, &event, &vk));
            assert(vk == 0);
        }
        assert(event_count == 1 && last_event == colors[i].user_event);
    }
    puts("Remote Home -> disconnect + LG home, Green -> Windows, Numpad 0-9, Yellow menu and Red disconnect passed");
    return 0;
}
