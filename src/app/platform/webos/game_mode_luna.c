#include "game_mode.h"
#include "game_mode_backend.h"

#include "lunasynccall.h"
#include "logging.h"

#include <pbnjson.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define HBCHANNEL_SERVICE_DIR \
    "/media/developer/apps/usr/palm/services/org.webosbrew.hbchannel.service"
#define HBCHANNEL_SERVICE_DIR_CRYPTO \
    "/media/cryptofs/apps/usr/palm/services/org.webosbrew.hbchannel.service"
#define URI_HBCHANNEL_EXEC "luna://org.webosbrew.hbchannel.service/exec"

static bool path_is_dir(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

bool webos_game_mode_is_rooted(void) {
    static int cached = -1;
    if (cached < 0) {
        cached = (path_is_dir(HBCHANNEL_SERVICE_DIR) || path_is_dir(HBCHANNEL_SERVICE_DIR_CRYPTO)) ? 1 : 0;
        commons_log_info("GameMode", "webosbrew root probe: %s",
                         cached ? "rooted (hbchannel service present)" : "not rooted");
    }
    return cached == 1;
}

static char *jstring_dup(jvalue_ref v) {
    if (!jis_string(v)) {
        return NULL;
    }
    raw_buffer buf = jstring_get(v);
    if (buf.m_str == NULL) {
        return NULL;
    }
    return strndup(buf.m_str, buf.m_len);
}

static bool json_return_value_true(const char *json) {
    if (json == NULL) {
        return false;
    }
    JSchemaInfo schema;
    jschema_info_init(&schema, jschema_all(), NULL, NULL);
    jdomparser_ref parser = jdomparser_create(&schema, 0);
    if (parser == NULL || !jdomparser_feed(parser, json, (int) strlen(json)) || !jdomparser_end(parser)) {
        if (parser) {
            jdomparser_release(&parser);
        }
        return false;
    }
    jvalue_ref root = jdomparser_get_result(parser);
    jvalue_ref rv = jobject_get(root, J_CSTR_TO_BUF("returnValue"));
    bool ok = false;
    if (jis_boolean(rv)) {
        jboolean_get(rv, &ok);
    }
    jdomparser_release(&parser);
    return ok;
}

static char *exec_root_cmd(const char *command) {
    jvalue_ref payload = jobject_create_var(
            jkeyval(J_CSTR_TO_JVAL("command"), jstring_create(command)),
            J_END_OBJ_DECL);
    if (!jis_valid(payload)) {
        return NULL;
    }
    const char *payload_str = jvalue_tostring_simple(payload);
    char *reply = NULL;
    bool called = HLunaServiceCallSync(URI_HBCHANNEL_EXEC, payload_str, true, &reply);
    j_release(&payload);
    if (!called || reply == NULL) {
        commons_log_warn("GameMode", "hbchannel exec call failed");
        free(reply);
        return NULL;
    }
    if (!json_return_value_true(reply)) {
        commons_log_warn("GameMode", "hbchannel exec rejected: %s", reply);
        free(reply);
        return NULL;
    }

    JSchemaInfo schema;
    jschema_info_init(&schema, jschema_all(), NULL, NULL);
    jdomparser_ref parser = jdomparser_create(&schema, 0);
    char *stdout_s = NULL;
    if (parser && jdomparser_feed(parser, reply, (int) strlen(reply)) && jdomparser_end(parser)) {
        jvalue_ref root = jdomparser_get_result(parser);
        stdout_s = jstring_dup(jobject_get(root, J_CSTR_TO_BUF("stdoutString")));
    }
    if (parser) {
        jdomparser_release(&parser);
    }
    free(reply);
    return stdout_s;
}

static char *exec_root_luna_uri(const char *uri, const char *inner_body_json) {
    /* Values read from firmware may contain apostrophes. Quote the JSON as
     * one shell argument before passing it through the Homebrew exec service. */
    char quoted[4096];
    size_t off = 0;
    quoted[off++] = '\'';
    for (const char *p = inner_body_json; *p; ++p) {
        const char *part = *p == '\'' ? "'\\''" : NULL;
        size_t len = part ? 4 : 1;
        if (off + len + 2 > sizeof(quoted)) return NULL;
        if (part) { memcpy(quoted + off, part, len); off += len; }
        else quoted[off++] = *p;
    }
    quoted[off++] = '\'';
    quoted[off] = '\0';
    char command[4608];
    int n = snprintf(command, sizeof(command), "luna-send -n 1 %s %s", uri, quoted);
    if (n < 0 || (size_t) n >= sizeof(command)) return NULL;
    return exec_root_cmd(command);
}

static char *exec_root_luna(const char *method, const char *inner_body_json) {
    char uri[256];
    snprintf(uri, sizeof(uri), "luna://com.webos.settingsservice/%s", method);
    return exec_root_luna_uri(uri, inner_body_json);
}

static char *parse_json_string_path(const char *json, const char *obj_key, const char *field) {
    if (json == NULL) {
        return NULL;
    }
    JSchemaInfo schema;
    jschema_info_init(&schema, jschema_all(), NULL, NULL);
    jdomparser_ref parser = jdomparser_create(&schema, 0);
    char *result = NULL;
    if (parser && jdomparser_feed(parser, json, (int) strlen(json)) && jdomparser_end(parser)) {
        jvalue_ref root = jdomparser_get_result(parser);
        jvalue_ref obj = jobject_get(root, j_cstr_to_buffer(obj_key));
        if (jis_object(obj)) {
            result = jstring_dup(jobject_get(obj, j_cstr_to_buffer(field)));
        }
    }
    if (parser) {
        jdomparser_release(&parser);
    }
    return result;
}

static char *get_config(const char *key) {
    char payload[384];
    snprintf(payload, sizeof(payload), "{\"configNames\":[\"%s\"]}", key);
    char *reply = NULL;
    if (HLunaServiceCallSync("luna://com.webos.service.config/getConfigs", payload, true, &reply) &&
        reply != NULL && json_return_value_true(reply)) {
        char *v = parse_json_string_path(reply, "configs", key);
        free(reply);
        reply = NULL;
        if (v != NULL) {
            return v;
        }
    }
    free(reply);
    char command[512];
    snprintf(command, sizeof(command),
             "luna-send -n 1 luna://com.webos.service.config/getConfigs '{\"configNames\":[\"%s\"]}'",
             key);
    char *out = exec_root_cmd(command);
    char *v = parse_json_string_path(out, "configs", key);
    free(out);
    return v;
}

static bool set_config(const char *key, const char *value) {
    jvalue_ref payload = jobject_create_var(
        jkeyval(J_CSTR_TO_JVAL("configs"), jobject_create_var(
            jkeyval(jstring_create(key), jstring_create(value)), J_END_OBJ_DECL)), J_END_OBJ_DECL);
    const char *body = jvalue_tostring_simple(payload);
    char *reply = NULL;
    bool ok = HLunaServiceCallSync("luna://com.webos.service.config/setConfigs", body, true, &reply) &&
        reply != NULL && json_return_value_true(reply);
    free(reply);
    if (!ok) {
        reply = exec_root_luna_uri("luna://com.webos.service.config/setConfigs", body);
        ok = json_return_value_true(reply);
        free(reply);
    }
    j_release(&payload);
    return ok;
}

char *webos_game_mode_get_setting(const char *category, const char *key) {
    if (category != NULL && strcmp(category, "config") == 0) {
        return get_config(key);
    }
    char body[320];
    snprintf(body, sizeof(body),
             "{\"category\":\"%s\",\"keys\":[\"%s\"]}", category, key);
    char *out = exec_root_luna("getSystemSettings", body);
    if (out == NULL || !json_return_value_true(out)) {
        snprintf(body, sizeof(body),
                 "{\"category\":\"%s\",\"keys\":[\"%s\"],\"current_app\":true}", category, key);
        free(out);
        out = exec_root_luna("getSystemSettings", body);
    }
    if (out == NULL || !json_return_value_true(out)) {
        free(out);
        return NULL;
    }
    JSchemaInfo schema;
    jschema_info_init(&schema, jschema_all(), NULL, NULL);
    jdomparser_ref parser = jdomparser_create(&schema, 0);
    char *result = NULL;
    if (parser && jdomparser_feed(parser, out, (int) strlen(out)) && jdomparser_end(parser)) {
        jvalue_ref root = jdomparser_get_result(parser);
        jvalue_ref settings = jobject_get(root, J_CSTR_TO_BUF("settings"));
        if (jis_object(settings)) {
            result = jstring_dup(jobject_get(settings, j_cstr_to_buffer(key)));
        }
    }
    if (parser) {
        jdomparser_release(&parser);
    }
    free(out);
    return result;
}

static bool set_setting_body(const char *body, bool log_fail) {
    char *out = exec_root_luna("setSystemSettings", body);
    if (out == NULL) {
        return false;
    }
    bool ok = json_return_value_true(out);
    if (!ok && log_fail) {
        commons_log_warn("GameMode", "setSystemSettings rejected: %s", out);
    }
    free(out);
    return ok;
}

bool webos_game_mode_set_setting(const char *category, const char *key, const char *value) {
    if (category != NULL && strcmp(category, "config") == 0) {
        return set_config(key, value);
    }
    jvalue_ref payload = jobject_create_var(
        jkeyval(J_CSTR_TO_JVAL("category"), jstring_create(category)),
        jkeyval(J_CSTR_TO_JVAL("settings"), jobject_create_var(
            jkeyval(jstring_create(key), jstring_create(value)), J_END_OBJ_DECL)), J_END_OBJ_DECL);
    bool ok = set_setting_body(jvalue_tostring_simple(payload), false);
    if (!ok) {
        jobject_put(payload, J_CSTR_TO_JVAL("current_app"), jboolean_create(true));
        ok = set_setting_body(jvalue_tostring_simple(payload), true);
    }
    j_release(&payload);
    return ok;
}
