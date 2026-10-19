#include "hints.h"
#include "util/i18n.h"

#include <SDL.h>

#define HINTS_LEN 4
static const char *hints_list[HINTS_LEN] = {
        translatable(
                "Long press BACK (or press EXIT for traditional TV remote) while streaming, to open status overlay."),
        translatable("Hold Select/Back alone for 4 seconds during streaming to show or hide performance stats."),
        translatable("Xbox One/Series controller can't be directly connected via cable or official adapter."),
        translatable(
                "If you have ultra-wide monitor, you may need to change its resolution to fit 16:9 for streaming."),
};

const char *hints_obtain() {
    static int hint_idx = -1;
    if (hint_idx < 0) {
        hint_idx = (int) SDL_GetTicks() % HINTS_LEN;
    } else if (++hint_idx >= HINTS_LEN) {
        hint_idx = 0;
    }

    return locstr(hints_list[hint_idx]);
}
