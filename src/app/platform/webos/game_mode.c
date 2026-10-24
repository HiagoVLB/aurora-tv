#include "game_mode.h"
#include "game_mode_backend.h"
#include "logging.h"
#include <stdlib.h>
#include <string.h>

typedef struct applied_setting {
    const char *category;
    const char *key;
    char *restore_to;
} applied_setting_t;

struct webos_game_mode_state {
    applied_setting_t *items;
    size_t count;
    size_t capacity;
    bool picture_mode_ok;
    bool hdr_active;
};

static bool state_push(webos_game_mode_state_t *state, const char *category, const char *key,
                       char *restore_to) {
    if (state->count >= state->capacity) {
        size_t ncap = state->capacity ? state->capacity * 2 : 4;
        applied_setting_t *ni = realloc(state->items, ncap * sizeof(*ni));
        if (ni == NULL) {
            free(restore_to);
            return false;
        }
        state->items = ni;
        state->capacity = ncap;
    }
    state->items[state->count++] = (applied_setting_t){
            .category = category,
            .key = key,
            .restore_to = restore_to,
    };
    return true;
}

static bool apply_one(webos_game_mode_state_t *state, const char *category, const char *key,
                      const char *value) {
    char *previous = webos_game_mode_get_setting(category, key);
    if (previous != NULL && strcmp(previous, value) == 0) {
        commons_log_info("GameMode", "%s.%s already %s", category, key, value);
        free(previous);
        return true;
    }
    /* Never change a TV setting unless its rollback is known and owned. */
    if (previous == NULL) {
        commons_log_warn("GameMode", "skipped %s.%s: previous value unavailable", category, key);
        return false;
    }
    bool tracked = false;
    for (size_t i = 0; i < state->count; i++) {
        if (strcmp(state->items[i].category, category) == 0 && strcmp(state->items[i].key, key) == 0) {
            tracked = true;
            break;
        }
    }
    if (!tracked && !state_push(state, category, key, previous)) return false;
    if (!webos_game_mode_set_setting(category, key, value)) {
        /* A timed-out reply may follow a successful firmware write. Retain
         * the original snapshot so teardown can still undo that write. */
        if (tracked) free(previous);
        commons_log_warn("GameMode", "failed to set %s.%s=%s", category, key, value);
        return false;
    }
    commons_log_info("GameMode", "%s.%s -> %s (was %s)", category, key, value, previous);
    if (tracked) free(previous);
    return true;
}

static bool apply_one_any(webos_game_mode_state_t *state, const char *category, const char *key,
                          const char *const *values, size_t nvalues, bool try_all) {
    for (size_t i = 0; i < nvalues; i++) {
        if (values[i] == NULL || values[i][0] == '\0') {
            continue;
        }
        if (apply_one(state, category, key, values[i])) {
            return true;
        }
        if (!try_all) {
            break;
        }
    }
    commons_log_info("GameMode", "skipped %s.%s (not in this firmware/context)", category, key);
    return false;
}

webos_game_mode_state_t *webos_game_mode_enter(void) {
    if (!webos_game_mode_is_rooted()) {
        return NULL;
    }
    webos_game_mode_state_t *state = calloc(1, sizeof(*state));
    if (state == NULL) {
        return NULL;
    }

    commons_log_info("GameMode", "app Game+IGR (not HDMI ALLM; overlay D= is NDL queue, unchanged)");
    /* Snapshot sound before picture. Switching pictureMode to game makes the TV
     * also flip soundMode to game; a get after that would save "game" and never
     * restore the user's previous sound. */
    char *sound_prev = webos_game_mode_get_setting("sound", "soundMode");
    if (sound_prev == NULL || !state_push(state, "sound", "soundMode", sound_prev)) {
        free(state);
        return NULL;
    }

    /* Connecting UI is still SDR. hdrGame is rejected ("no matched extended item")
     * until HDR actually engages — start with SDR Game, then HDR aliases. */
    static const char *pic_sdr[] = {"game", "hdrGame", "dolbyHdrGame"};
    state->picture_mode_ok = apply_one_any(state, "picture", "pictureMode",
                                           pic_sdr, sizeof(pic_sdr) / sizeof(pic_sdr[0]), true);

    /* The original sound snapshot above predates picture-mode side effects. */
    if (!webos_game_mode_set_setting("sound", "soundMode", "game")) {
        commons_log_warn("GameMode", "failed to set sound.soundMode=game");
    }

    if (state->picture_mode_ok) {
        static const char *off_vals[] = {"off"};
        static const char *on_vals[] = {"on"};
        apply_one_any(state, "picture", "truMotionMode", off_vals, 1, false);
        apply_one_any(state, "picture", "instantGameResponse", on_vals, 1, false);
        apply_one_any(state, "picture", "gameOptimiser", on_vals, 1, false);
    }

    return state;
}

void webos_game_mode_on_hdr(webos_game_mode_state_t *state, bool hdr) {
    if (state == NULL || hdr == state->hdr_active) {
        return;
    }
    if (!hdr) {
        if (apply_one(state, "picture", "pictureMode", "game")) state->hdr_active = false;
        return;
    }
    static const char *pic_hdr[] = {"hdrGame", "dolbyHdrGame"};
    if (apply_one_any(state, "picture", "pictureMode", pic_hdr, 2, true)) {
        static const char *peak[] = {"high", "medium"};
        apply_one_any(state, "picture", "peakBrightness", peak, 2, false);
        state->picture_mode_ok = true;
        state->hdr_active = true;
    }
}

static void restore_matching(webos_game_mode_state_t *state, const char *category) {
    for (size_t i = state->count; i > 0; i--) {
        applied_setting_t *a = &state->items[i - 1];
        if (a->restore_to == NULL || strcmp(a->category, category) != 0) {
            continue;
        }
        if (webos_game_mode_set_setting(a->category, a->key, a->restore_to)) {
            commons_log_info("GameMode", "restored %s.%s=%s", a->category, a->key, a->restore_to);
        } else {
            commons_log_warn("GameMode", "restore failed %s.%s=%s", a->category, a->key,
                             a->restore_to);
        }
        free(a->restore_to);
        a->restore_to = NULL;
    }
}

void webos_game_mode_restore(webos_game_mode_state_t *state) {
    if (state == NULL) {
        return;
    }
    /* Picture first. Restoring sound while picture is still Game makes the TV
     * immediately force soundMode back to game (C5). */
    char *sound_again = NULL;
    const char *sound_key = "soundMode";
    for (size_t i = 0; i < state->count; i++) {
        if (strcmp(state->items[i].category, "sound") == 0 && state->items[i].restore_to != NULL) {
            sound_again = strdup(state->items[i].restore_to);
            sound_key = state->items[i].key;
            break;
        }
    }
    restore_matching(state, "picture");
    restore_matching(state, "sound");
    if (sound_again != NULL) {
        if (webos_game_mode_set_setting("sound", sound_key, sound_again)) {
            commons_log_info("GameMode", "sound.%s restore retry=%s", sound_key, sound_again);
        }
        free(sound_again);
    }
    restore_matching(state, "config");
    for (size_t i = 0; i < state->count; i++) {
        applied_setting_t *a = &state->items[i];
        if (a->restore_to == NULL) {
            continue;
        }
        if (webos_game_mode_set_setting(a->category, a->key, a->restore_to)) {
            commons_log_info("GameMode", "restored %s.%s=%s", a->category, a->key, a->restore_to);
        }
        free(a->restore_to);
        a->restore_to = NULL;
    }
    free(state->items);
    free(state);
}
