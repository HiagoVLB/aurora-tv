/* Exercise webOS caching/lifetime with SDL threads and the allocating firmware API. */
#include "util/i18n.h"
#include "logging.h"
#include "webosi18n_C.h"
#include <SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ResBundleC { char locale[16]; };
static unsigned lookups;
static SDL_atomic_t stop;

ResBundleC *resBundle_createWithRootPath(const char *locale, const char *file, const char *root) {
    (void)file; (void)root;
    if (strcmp(locale, "pt-BR") && strcmp(locale, "fr")) return NULL;
    ResBundleC *b = calloc(1, sizeof(*b));
    assert(b);
    SDL_strlcpy(b->locale, locale, sizeof(b->locale));
    return b;
}
const char *resBundle_getLocString(ResBundleC *b, const char *msgid) {
    lookups++;
    if (!strcmp(msgid, "missing")) return NULL;
    char text[128];
    snprintf(text, sizeof(text), "%s:%s", b->locale, msgid);
    return strdup(text);
}
void resBundle_destroy(ResBundleC *b) { free(b); }
void commons_log_printf(commons_log_level level, const char *tag, const char *fmt, ...) {
    (void)level; (void)tag; (void)fmt;
}
static int lookup_worker(void *unused) {
    (void)unused;
    while (!SDL_AtomicGet(&stop)) {
        const char *s = locstr("Cancel");
        assert(!strcmp(s, "pt-BR:Cancel") || !strcmp(s, "fr:Cancel"));
    }
    return 0;
}
int main(void) {
    assert(SDL_Init(0) == 0);
    assert(!strcmp(locstr("Cancel"), "Cancel"));
    i18n_setlocale("pt-BR");
    assert(i18n_is_loaded());
    char key[] = "Cancel";
    const char *pt = locstr(key);
    assert(!strcmp(pt, "pt-BR:Cancel"));
    strcpy(key, "unused"); /* Cache keys must be copied and compared by content. */
    unsigned before = lookups;
    for (int i = 0; i < 10000; i++) assert(locstr("Cancel") == pt);
    assert(lookups == before);
    assert(!strcmp(locstr("missing"), "missing"));
    i18n_setlocale("fr");
    const char *fr = locstr("Cancel");
    assert(!strcmp(fr, "fr:Cancel") && !strcmp(pt, "pt-BR:Cancel"));
    i18n_setlocale("pt-BR");
    assert(locstr("Cancel") == pt);
    SDL_Thread *threads[4];
    for (int i = 0; i < 4; i++) {
        threads[i] = SDL_CreateThread(lookup_worker, "locstr-test", NULL);
        assert(threads[i]);
    }
    before = lookups;
    for (int i = 0; i < 200; i++) i18n_setlocale(i % 2 ? "pt-BR" : "fr");
    SDL_AtomicSet(&stop, 1);
    for (int i = 0; i < 4; i++) SDL_WaitThread(threads[i], NULL);
    assert(lookups == before + 200); /* Only bundle probes; no new cached strings. */
    i18n_setlocale("invalid");
    assert(!i18n_is_loaded() && !strcmp(i18n_locale(), ""));
    assert(!strcmp(locstr("Cancel"), "Cancel"));
    assert(!strcmp(pt, "pt-BR:Cancel") && !strcmp(fr, "fr:Cancel"));
    i18n_setlocale("pt-BR");
    assert(locstr("Cancel") == pt);
    i18n_setlocale("auto");
    assert(!i18n_is_loaded());
    SDL_Quit();
    puts("webOS translations: bounded lookups, locale reuse, retained labels, fallback and concurrent reload passed");
    return 0;
}
