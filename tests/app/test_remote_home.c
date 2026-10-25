/* Run the actual webOS remote mapping with firmware/UI entry points stubbed. */
#define app_webos_open_ribbon test_open_ribbon
#define streaming_soft_keyboard_shown test_keyboard_shown
#define ui_should_block_input test_block_input
#define bus_pushevent test_post_event
#include "platform/webos/sdl_webos_scancode.h"
#include "../../src/app/platform/webos/keyboard_webos.c"
#include <assert.h>
#include <stdio.h>

static int ribbon_count;
void test_open_ribbon(void) { ribbon_count++; }
bool test_keyboard_shown(void) { return false; }
bool test_block_input(void) { return false; }
bool test_post_event(int which, void *data1, void *data2) { return true; }

int main(void) {
    session_t session = {0};
    stream_input_t input = {.session = &session};
    SDL_KeyboardEvent event = {.keysym.scancode = SDL_SCANCODE_WEBOS_HOME};
    short vk;
    /* Both edges must reach the host, independent of system-key capture. */
    for (int down = 0; down <= 1; down++) {
        event.state = down ? SDL_PRESSED : SDL_RELEASED;
        vk = 0;
        assert(!stream_input_webos_intercept_remote_keys(&input, &event, &vk));
        assert(vk == VK_LWIN);
    }
    assert(ribbon_count == 0);
    /* View-only and idle keep the TV Home behavior, once on release. */
    for (int idle = 0; idle <= 1; idle++) {
        input.session = idle ? NULL : &session;
        input.view_only = !idle;
        event.state = SDL_PRESSED;
        vk = 0;
        assert(stream_input_webos_intercept_remote_keys(&input, &event, &vk));
        assert(vk == 0 && ribbon_count == idle);
        event.state = SDL_RELEASED;
        assert(stream_input_webos_intercept_remote_keys(&input, &event, &vk));
        assert(ribbon_count == idle + 1);
    }
    /* Physical keyboard Home is handled by the regular keyboard mapper. */
    event.keysym.scancode = SDL_SCANCODE_HOME;
    vk = 0;
    assert(!stream_input_webos_intercept_remote_keys(&input, &event, &vk));
    assert(vk == 0);
    /* webOS delivers remote digits as SDL 0..9; preserve both key edges. */
    input.session = &session;
    input.view_only = false;
    for (int digit = 0; digit <= 9; digit++) {
        event.keysym.scancode = digit == 0 ? SDL_SCANCODE_0 : SDL_SCANCODE_1 + digit - 1;
        for (int down = 0; down <= 1; down++) {
            event.state = down ? SDL_PRESSED : SDL_RELEASED;
            vk = 0;
            assert(!stream_input_webos_intercept_remote_keys(&input, &event, &vk));
            assert(vk == VK_NUMPAD0 + digit);
        }
    }
    puts("Remote Home/Windows, idle/view-only ribbon, physical Home and Numpad 0-9 passed");
    return 0;
}
