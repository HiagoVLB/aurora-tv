/*
 * This file is part of Moonlight Embedded.
 *
 * Copyright (C) 2015 Iwan Timmer
 *
 * Moonlight is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Moonlight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Moonlight; if not, see <http://www.gnu.org/licenses/>.
 */

#include "xml.h"
#include "errors.h"
#include "set_error.h"

#include <expat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define STATUS_OK 200

struct xml_query {
    char *memory;
    size_t size;
    int start;
    void *data;
};

static void XMLCALL start_element(void *userData, const char *name, const char **atts);

static void XMLCALL end_element(void *userData, const char *name);

static void XMLCALL start_applist_element(void *userData, const char *name, const char **atts);

static void XMLCALL end_applist_element(void *userData, const char *name);

static void XMLCALL start_mode_element(void *userData, const char *name, const char **atts);

static void XMLCALL end_mode_element(void *userData, const char *name);

static void XMLCALL start_status_element(void *userData, const char *name, const char **atts);

static void XMLCALL end_status_element(void *userData, const char *name);

static void XMLCALL write_cdata(void *userData, const XML_Char *s, int len);

int xml_search(char *data, size_t len, const char *node, char **result) {
    return xml_search_ex(data, len, node, false, result);
}

int xml_search_ex(char *data, size_t len, const char *node, bool required, char **result) {
    struct xml_query search = {.memory = required ? NULL : calloc(1, 1), .data = (void *) node};
    XML_Parser parser = XML_ParserCreate("UTF-8");
    XML_SetUserData(parser, &search);
    XML_SetElementHandler(parser, start_element, end_element);
    XML_SetCharacterDataHandler(parser, write_cdata);
    if (!XML_Parse(parser, data, (int) len, 1)) {
        int code = XML_GetErrorCode(parser);
        const char *error = XML_ErrorString(code);
        XML_ParserFree(parser);
        if (search.memory != NULL) {
            free(search.memory);
        }
        return gs_set_error(GS_INVALID, "XML error %d: %s", code, error);
    }

    XML_ParserFree(parser);
    if (search.memory == NULL && required) {
        return gs_set_error(GS_INVALID, "Cant find value for `%s`", node);
    }
    *result = search.memory;
    return GS_OK;
}

int xml_applist(char *data, size_t len, PAPP_LIST *app_list) {
    struct xml_query query = {0};
    XML_Parser parser = XML_ParserCreate("UTF-8");
    XML_SetUserData(parser, &query);
    XML_SetElementHandler(parser, start_applist_element, end_applist_element);
    XML_SetCharacterDataHandler(parser, write_cdata);
    if (!XML_Parse(parser, data, (int) len, 1)) {
        int code = XML_GetErrorCode(parser);
        const char *error = XML_ErrorString(code);
        XML_ParserFree(parser);
        if (query.memory != NULL) {
            free(query.memory);
        }
        return gs_set_error(GS_INVALID, "XML error %d: %s", code, error);
    }

    XML_ParserFree(parser);
    if (query.memory != NULL) {
        free(query.memory);
    }
    *app_list = (PAPP_LIST) query.data;

    return GS_OK;
}

int xml_modelist(char *data, size_t len, PDISPLAY_MODE *mode_list) {
    struct xml_query query = {0};
    XML_Parser parser = XML_ParserCreate("UTF-8");
    XML_SetUserData(parser, &query);
    XML_SetElementHandler(parser, start_mode_element, end_mode_element);
    XML_SetCharacterDataHandler(parser, write_cdata);
    if (!XML_Parse(parser, data, (int) len, 1)) {
        int code = XML_GetErrorCode(parser);
        const char *error = XML_ErrorString(code);
        XML_ParserFree(parser);
        if (query.memory != NULL) {
            free(query.memory);
        }
        return gs_set_error(GS_INVALID, "XML error %d: %s", code, error);
    }

    XML_ParserFree(parser);
    if (query.memory != NULL) {
        free(query.memory);
    }
    *mode_list = (PDISPLAY_MODE) query.data;

    return GS_OK;

}

typedef struct {
    int code;
    char message[1024];
} xml_status_result_t;

int xml_status(char *data, size_t len) {
    return xml_status_ex(data, len, NULL, NULL, 0);
}

int xml_status_ex(char *data, size_t len, int *status_code, char *message, size_t message_size) {
    xml_status_result_t status = {0};
    XML_Parser parser = XML_ParserCreate("UTF-8");
    XML_SetUserData(parser, &status);
    XML_SetElementHandler(parser, start_status_element, end_status_element);
    if (!XML_Parse(parser, data, (int) len, 1)) {
        int code = XML_GetErrorCode(parser);
        const char *error = XML_ErrorString(code);
        XML_ParserFree(parser);
        return gs_set_error(GS_INVALID, "XML error %d: %s", code, error);
    }

    XML_ParserFree(parser);
    if (status_code) *status_code = status.code;
    if (message && message_size) snprintf(message, message_size, "%s", status.message);
    if (status.code != STATUS_OK) {
        gs_set_error(GS_ERROR, "Bad status %d: %s", status.code, status.message);
    }
    return status.code == STATUS_OK ? GS_OK : GS_ERROR;
}

void start_element(void *userData, const char *name, const char **atts) {
    struct xml_query *search = (struct xml_query *) userData;
    if (strcmp(search->data, name) == 0) {
        search->start++;
    }
}

void end_element(void *userData, const char *name) {
    struct xml_query *search = (struct xml_query *) userData;
    if (strcmp(search->data, name) == 0) {
        search->start--;
    }
}

void start_applist_element(void *userData, const char *name, const char **atts) {
    struct xml_query *search = (struct xml_query *) userData;
    if (strcmp("App", name) == 0) {
        PAPP_LIST app = malloc(sizeof(APP_LIST));
        if (app == NULL) {
            return;
        }

        app->id = 0;
        app->name = NULL;
        app->hdr = 0;
        app->uuid[0] = 0;
        app->art_version[0] = 0;
        app->next = (PAPP_LIST) search->data;
        search->data = app;
    } else if (strcmp("ID", name) == 0 || strcmp("AppTitle", name) == 0 || strcmp("IsHdrSupported", name) == 0 ||
               strcmp("UUID", name) == 0 || strcmp("ArtVersion", name) == 0) {
        search->memory = malloc(1);
        search->size = 0;
        search->start = 1;
    }
}

void end_applist_element(void *userData, const char *name) {
    struct xml_query *search = (struct xml_query *) userData;
    if (search->start) {
        PAPP_LIST list = (PAPP_LIST) search->data;
        if (list == NULL) {
            return;
        }

        if (strcmp("ID", name) == 0) {
            list->id = (int) strtol(search->memory, NULL, 10);
            free(search->memory);
            search->memory = NULL;
        } else if (strcmp("AppTitle", name) == 0) {
            list->name = search->memory;
            search->memory = NULL;
        } else if (strcmp("UUID", name) == 0 || strcmp("ArtVersion", name) == 0) {
            if (strcmp("UUID", name) == 0) {
                snprintf(list->uuid, sizeof(list->uuid), "%s", search->memory);
            } else {
                snprintf(list->art_version, sizeof(list->art_version), "%s", search->memory);
            }
            free(search->memory);
            search->memory = NULL;
        } else if (strcmp("IsHdrSupported", name) == 0) {
            list->hdr = (int) strtol(search->memory, NULL, 10);
            free(search->memory);
            search->memory = NULL;
        }
        search->start = 0;
    }
}

void start_mode_element(void *userData, const char *name, const char **atts) {
    struct xml_query *search = (struct xml_query *) userData;
    if (strcmp("DisplayMode", name) == 0) {
        PDISPLAY_MODE mode = calloc(1, sizeof(DISPLAY_MODE));
        if (mode != NULL) {
            mode->next = (PDISPLAY_MODE) search->data;
            search->data = mode;
        }
    } else if (search->data != NULL &&
               (strcmp("Height", name) == 0 || strcmp("Width", name) == 0 || strcmp("RefreshRate", name) == 0)) {
        search->memory = malloc(1);
        search->size = 0;
        search->start = 1;
    }
}

void end_mode_element(void *userData, const char *name) {
    struct xml_query *search = (struct xml_query *) userData;
    if (search->data == NULL || !search->start) {
        return;
    }
    PDISPLAY_MODE mode = (PDISPLAY_MODE) search->data;
    if (strcmp("Width", name) == 0) {
        mode->width = strtol(search->memory, NULL, 10);
    } else if (strcmp("Height", name) == 0) {
        mode->height = strtol(search->memory, NULL, 10);
    } else if (strcmp("RefreshRate", name) == 0) {
        mode->refresh = strtol(search->memory, NULL, 10);
    }

    free(search->memory);
    search->memory = NULL;
    search->start = 0;
}

void start_status_element(void *userData, const char *name, const char **atts) {
    if (strcmp("root", name) != 0) {
        return;
    }
    xml_status_result_t *status = userData;
    for (int i = 0; atts[i]; i += 2) {
        if (strcmp("status_code", atts[i]) == 0) {
            status->code = atoi(atts[i + 1]);
        } else if (strcmp("status_message", atts[i]) == 0) {
            snprintf(status->message, sizeof(status->message), "%s", atts[i + 1]);
        }
    }
}

void end_status_element(void *userData, const char *name) {}

void write_cdata(void *userData, const XML_Char *s, int len) {
    struct xml_query *search = (struct xml_query *) userData;
    if (search->start <= 0) {
        return;
    }
    void *allocated = realloc(search->memory, search->size + len + 1);
    assert(allocated != NULL);
    search->memory = allocated;

    memcpy(search->memory + search->size, s, len);
    search->size += len;
    search->memory[search->size] = 0;
}

bool xml_app_is_remote_control(const APP_LIST *app) {
    static const char prefix[] = "9a1c5a25-58fe-40e0-b9aa-7d3f0000000";
    return app && strlen(app->uuid) == 36 && strncmp(app->uuid, prefix, 35) == 0 &&
           app->uuid[35] >= '1' && app->uuid[35] <= '7';
}

bool xml_app_is_remote_display(const APP_LIST *app) {
    return xml_app_is_remote_control(app) && (app->uuid[35] == '5' || app->uuid[35] == '6');
}

typedef struct {
    int depth;
    bool root_seen, root_ok, started, bad;
    int field; /* 0 none, 1 code, 2 message, 3 gamesession/resume */
    int code_count, message_count;
    char code[GS_LAUNCH_WARNING_CODE_MAX + 1];
    size_t code_len;
    char message[GS_LAUNCH_WARNING_MESSAGE_MAX * 2 + 1];
    size_t message_len;
    bool message_too_long;
    char flag[8];
    size_t flag_len;
} launch_warning_state_t;

static void XMLCALL lw_start(void *user, const char *name, const char **atts) {
    launch_warning_state_t *st = user;
    st->depth++;
    st->field = 0;
    if (st->depth == 1) {
        if (st->root_seen || strcmp(name, "root") != 0) { st->bad = true; return; }
        st->root_seen = true;
        for (int i = 0; atts && atts[i]; i += 2) {
            if (strcmp(atts[i], "status_code") == 0 && strcmp(atts[i + 1], "200") == 0) st->root_ok = true;
        }
    } else if (st->depth == 2) {
        if (strcmp(name, "clientWarning") == 0) {
            if (++st->code_count > 1) st->bad = true;
            st->field = 1;
        } else if (strcmp(name, "clientWarningMessage") == 0) {
            if (++st->message_count > 1) st->bad = true;
            st->field = 2;
        } else if (strcmp(name, "gamesession") == 0 || strcmp(name, "resume") == 0) {
            st->field = 3;
            st->flag_len = 0;
        }
    }
}

static void XMLCALL lw_end(void *user, const char *name) {
    (void) name;
    launch_warning_state_t *st = user;
    if (st->depth == 2 && st->field == 3) {
        st->flag[st->flag_len] = 0;
        if (strcmp(st->flag, "1") == 0) st->started = true;
    }
    st->field = 0;
    st->depth--;
}

static void XMLCALL lw_cdata(void *user, const XML_Char *s, int len) {
    launch_warning_state_t *st = user;
    if (st->depth != 2 || len <= 0) return;
    if (st->field == 1) {
        if (st->code_len + (size_t) len > GS_LAUNCH_WARNING_CODE_MAX) { st->bad = true; return; }
        memcpy(st->code + st->code_len, s, (size_t) len);
        st->code_len += (size_t) len;
    } else if (st->field == 2) {
        if (st->message_len + (size_t) len > sizeof(st->message) - 1) { st->message_too_long = true; return; }
        memcpy(st->message + st->message_len, s, (size_t) len);
        st->message_len += (size_t) len;
    } else if (st->field == 3) {
        size_t n = (size_t) len;
        if (st->flag_len + n > sizeof(st->flag) - 1) n = sizeof(st->flag) - 1 - st->flag_len;
        memcpy(st->flag + st->flag_len, s, n);
        st->flag_len += n;
    }
}

/* Decode one UTF-8 code point; returns its length or 0 when invalid. */
static size_t lw_utf8(const unsigned char *p, size_t left, unsigned int *cp) {
    if (p[0] < 0x80) { *cp = p[0]; return 1; }
    size_t n = (p[0] & 0xE0) == 0xC0 ? 2 : (p[0] & 0xF0) == 0xE0 ? 3 : (p[0] & 0xF8) == 0xF0 ? 4 : 0;
    if (n == 0 || n > left) return 0;
    unsigned int v = p[0] & (0x7F >> n);
    for (size_t i = 1; i < n; i++) {
        if ((p[i] & 0xC0) != 0x80) return 0;
        v = (v << 6) | (p[i] & 0x3F);
    }
    *cp = v;
    return n;
}

static bool lw_drop(unsigned int cp) {
    if (cp < 0x20 || (cp >= 0x7F && cp < 0xA0)) return true;          /* C0/C1 controls */
    if (cp == 0x061C || cp == 0x200E || cp == 0x200F) return true;    /* bidi marks */
    if (cp >= 0x202A && cp <= 0x202E) return true;                    /* bidi embeddings */
    if (cp >= 0x2060 && cp <= 0x206F) return true;                    /* invisible/isolates */
    if (cp == 0xFEFF || (cp >= 0xFFF9 && cp <= 0xFFFB)) return true;  /* BOM, annotations */
    return false; /* ZWNJ/ZWJ (0x200C/0x200D) are kept for emoji and scripts. */
}

static void lw_sanitize(const char *in, size_t in_len, char *out, size_t out_size) {
    size_t o = 0;
    int breaks = 0;
    out[0] = 0;
    for (size_t i = 0; i < in_len;) {
        unsigned int cp;
        size_t n = lw_utf8((const unsigned char *) in + i, in_len - i, &cp);
        if (n == 0) { i++; continue; }
        const char *piece = in + i;
        size_t piece_len = n;
        i += n;
        if (cp == '\n' || cp == 0x2028 || cp == 0x2029) {
            if (++breaks > 3) { piece = " "; piece_len = 1; } else { piece = "\n"; piece_len = 1; }
        } else if (cp == '\r' || cp == '\t') {
            piece = " "; piece_len = 1;
        } else if (lw_drop(cp)) {
            continue;
        }
        if (o + piece_len > GS_LAUNCH_WARNING_MESSAGE_MAX || o + piece_len >= out_size) { out[0] = 0; return; }
        memcpy(out + o, piece, piece_len);
        o += piece_len;
    }
    /* Trim surrounding whitespace. */
    size_t start = 0;
    while (start < o && (out[start] == ' ' || out[start] == '\n')) start++;
    while (o > start && (out[o - 1] == ' ' || out[o - 1] == '\n')) o--;
    memmove(out, out + start, o - start);
    out[o - start] = 0;
}

bool xml_launch_warning(const char *data, size_t len, char *code, size_t code_size, char *message,
                        size_t message_size) {
    if (code && code_size) code[0] = 0;
    if (message && message_size) message[0] = 0;
    if (data == NULL || len == 0 || len > 16384 || code == NULL || code_size == 0) return false;
    launch_warning_state_t *st = calloc(1, sizeof(*st));
    if (st == NULL) return false;
    XML_Parser parser = XML_ParserCreate("UTF-8");
    if (parser == NULL) { free(st); return false; }
    XML_SetUserData(parser, st);
    XML_SetElementHandler(parser, lw_start, lw_end);
    XML_SetCharacterDataHandler(parser, lw_cdata);
    bool ok = XML_Parse(parser, data, (int) len, 1) == XML_STATUS_OK;
    XML_ParserFree(parser);
    bool found = false;
    if (ok && !st->bad && st->root_ok && st->started && st->code_count == 1 && st->code_len > 0) {
        st->code[st->code_len] = 0;
        bool code_ok = true;
        for (size_t i = 0; i < st->code_len; i++) {
            char c = st->code[i];
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' ||
                  c == '.')) {
                code_ok = false;
            }
        }
        if (code_ok) {
            snprintf(code, code_size, "%s", st->code);
            if (message && message_size && st->message_count == 1 && !st->message_too_long) {
                lw_sanitize(st->message, st->message_len, message, message_size);
            }
            found = true;
        }
    }
    free(st);
    return found;
}
