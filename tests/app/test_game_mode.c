/* Exercise the real rollback policy with firmware failures and picture/sound side effects. */
#include "platform/webos/game_mode.h"
#include "platform/webos/game_mode_backend.h"
#include "logging.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static struct { const char *key; char value[40]; } values[] = {
    {"pictureMode", "cinema"}, {"soundMode", "music"}, {"truMotionMode", "smooth"},
    {"instantGameResponse", "off"}, {"gameOptimiser", "off"}, {"peakBrightness", "low"}
};
static const char *unreadable, *rejected;
static int writes;
static bool rooted = true;
static bool fail_after_write;
bool webos_game_mode_is_rooted(void) { return rooted; }
char *webos_game_mode_get_setting(const char *category, const char *key) {
    (void)category;
    if (unreadable && strcmp(key, unreadable) == 0) return NULL;
    for (size_t i = 0; i < sizeof(values)/sizeof(*values); i++)
        if (strcmp(key, values[i].key) == 0) return strdup(values[i].value);
    return NULL;
}
bool webos_game_mode_set_setting(const char *category, const char *key, const char *value) {
    (void)category;
    writes++;
    bool fail = rejected && strcmp(key, rejected) == 0;
    if (fail && !fail_after_write) return false;
    for (size_t i = 0; i < sizeof(values)/sizeof(*values); i++) {
        if (strcmp(key, values[i].key) != 0) continue;
        snprintf(values[i].value, sizeof(values[i].value), "%s", value);
        if (strcmp(key, "pictureMode") == 0) strcpy(values[1].value, "game");
        return !fail;
    }
    return false;
}
void commons_log_printf(commons_log_level level, const char *tag, const char *fmt, ...) {
    (void)level; (void)tag; (void)fmt;
}
static void assert_original(void) {
    assert(strcmp(values[0].value, "cinema") == 0);
    assert(strcmp(values[1].value, "music") == 0);
    assert(strcmp(values[2].value, "smooth") == 0);
    assert(strcmp(values[3].value, "off") == 0);
    assert(strcmp(values[4].value, "off") == 0);
    assert(strcmp(values[5].value, "low") == 0);
}
int main(void) {
    rooted = false;
    assert(webos_game_mode_enter() == NULL && writes == 0);
    rooted = true;
    unreadable = "soundMode";
    assert(webos_game_mode_enter() == NULL && writes == 0);
    unreadable = "truMotionMode";
    webos_game_mode_state_t *s = webos_game_mode_enter();
    assert(s && strcmp(values[2].value, "smooth") == 0);
    webos_game_mode_restore(s);
    assert_original();
    unreadable = NULL;
    rejected = "instantGameResponse";
    s = webos_game_mode_enter();
    assert(s && strcmp(values[3].value, "off") == 0);
    rejected = NULL;
    webos_game_mode_on_hdr(s, true);
    assert(strcmp(values[0].value, "hdrGame") == 0);
    int before = writes;
    for (int i = 0; i < 100; i++) webos_game_mode_on_hdr(s, true);
    assert(writes == before);
    for (int i = 0; i < 100; i++) {
        webos_game_mode_on_hdr(s, false);
        webos_game_mode_on_hdr(s, true);
    }
    webos_game_mode_restore(s);
    assert_original();
    unreadable = "pictureMode";
    s = webos_game_mode_enter();
    assert(s && strcmp(values[0].value, "cinema") == 0);
    webos_game_mode_restore(s);
    assert_original();
    unreadable = NULL;
    rejected = "instantGameResponse";
    fail_after_write = true;
    s = webos_game_mode_enter();
    assert(s && strcmp(values[3].value, "on") == 0);
    rejected = NULL;
    webos_game_mode_restore(s);
    assert_original();
    webos_game_mode_restore(NULL);
    puts("Game Mode rollback and firmware failure checks passed");
    return 0;
}
