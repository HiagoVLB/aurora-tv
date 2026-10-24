/* Test the actual editor parser under a locale that uses decimal commas. */
#include "../../src/app/ui/settings/panes/pref_fps.c"
#include <assert.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>

int main(void) {
    const char *locale = setlocale(LC_NUMERIC, "pt_BR.UTF-8");
    if (!locale) puts("Comma locale unavailable; testing C locale");
    double fps;
    assert(parse_custom_fps("119.88", &fps) && fabs(fps - 119.88) < 0.00001);
    assert(parse_custom_fps("59.94", &fps) && fabs(fps - 59.94) < 0.00001);
    assert(parse_custom_fps("120", &fps) && fps == 120);
    assert(!parse_custom_fps("", &fps));
    assert(!parse_custom_fps("59.9.4", &fps));
    assert(!parse_custom_fps("NaN", &fps));
    assert(!parse_custom_fps("Infinity", &fps));
    puts("Fractional FPS editor locale checks passed");
    return 0;
}
