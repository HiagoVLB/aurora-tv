/* Exercises real client/XML parsing with an HTTP fixture, without a live host. */
#include "client.h"
#include "http.h"
#include "errors.h"
#include "uuidstr.h"
#include "priv.h"
#include "logging.h"
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *reply;
static int transport_status;
static char last_url[4096];
/* Deterministic crypto is confined to this test executable; test no crypto here. */
void mbedtls_entropy_init(mbedtls_entropy_context *ctx) { (void) ctx; }
void mbedtls_entropy_free(mbedtls_entropy_context *ctx) { (void) ctx; }
int mbedtls_entropy_func(void *ctx, unsigned char *out, size_t len) { (void) ctx; memset(out, 0, len); return 0; }
void mbedtls_ctr_drbg_init(mbedtls_ctr_drbg_context *ctx) { (void) ctx; }
void mbedtls_ctr_drbg_free(mbedtls_ctr_drbg_context *ctx) { (void) ctx; }
int mbedtls_ctr_drbg_seed(mbedtls_ctr_drbg_context *ctx, int (*rng)(void *, unsigned char *, size_t), void *entropy,
                         const unsigned char *custom, size_t len) {
    (void) ctx; (void) rng; (void) entropy; (void) custom; (void) len; return 0;
}
int mbedtls_ctr_drbg_random(void *ctx, unsigned char *out, size_t len) { return mbedtls_entropy_func(ctx, out, len); }
void http_set_timeout(HTTP *http, int timeout) { (void) http; (void) timeout; }
void commons_log_printf(commons_log_level level, const char *tag, const char *fmt, ...) {
    (void) level; (void) tag; (void) fmt;
}
const char *LiGetLaunchUrlQueryParameters(void) { return ""; }

HTTP_DATA *http_data_alloc(void) { return calloc(1, sizeof(HTTP_DATA)); }
void http_data_free(HTTP_DATA *data) { if (data) { free(data->memory); free(data); } }
int http_request(HTTP *http, char *url, HTTP_DATA *data) {
    (void) http;
    snprintf(last_url, sizeof(last_url), "%s", url);
    assert(strstr(url, "https://fixture:47984/") != NULL);
    if (transport_status != GS_OK) return transport_status;
    data->memory = strdup(reply);
    data->size = strlen(reply);
    return GS_OK;
}
bool uuidstr_random(uuidstr_t *dest) {
    memcpy(dest, "00000000-0000-4000-8000-000000000000", 37);
    return true;
}

int main(void) {
    struct GS_CLIENT_T client = {0};
    SERVER_DATA server = {.serverInfo.address = "fixture", .currentGame = 42};
    int applied = -1;
    reply = "<root status_code=\"200\"><bitrate>45000</bitrate></root>";
    assert(gs_set_bitrate_ex(&client, &server, 93000, &applied) == GS_OK);
    assert(applied == 45000); /* Host clamp must replace the requested value. */
    const char *invalid[] = {
        "<root status_code=\"404\" status_message=\"No active session\"><bitrate>0</bitrate></root>",
        "<root status_code=\"403\"><bitrate>0</bitrate></root>",
        "<root status_code=\"200\"><bitrate>0</bitrate></root>",
        "<root status_code=\"200\"><bitrate>90000junk</bitrate></root>",
        "<root status_code=\"200\"><bitrate>9999999999999999999999999999</bitrate></root>",
        "<root status_code=\"200\"></root>", "<root"
    };
    for (size_t i = 0; i < sizeof(invalid)/sizeof(*invalid); i++) {
        reply = invalid[i]; applied = -1;
        assert(gs_set_bitrate_ex(&client, &server, 93000, &applied) != GS_OK);
        assert(applied == -1);
    }
    transport_status = GS_IO_ERROR;
    assert(gs_set_bitrate(&client, &server, 93000) == GS_IO_ERROR);
    transport_status = GS_OK;
    reply = "<root status_code=\"403\"><cancel>0</cancel></root>";
    assert(gs_quit_app(&client, &server) == GS_ERROR);
    assert(server.currentGame == 42);
    reply = "<root status_code=\"200\"><cancel>1</cancel></root>";
    assert(gs_quit_app(&client, &server) == GS_OK && server.currentGame == 0);
    char catalogue[] = "<root status_code=\"200\"><App><UUID>9a1c5a25-58fe-40e0-b9aa-7d3f00000005</UUID><ID>123</ID><ArtVersion>remote-session-v6</ArtVersion><IsHdrSupported>1</IsHdrSupported><AppTitle>Remote Monitor</AppTitle></App><App><ID>9</ID><AppTitle>Desktop</AppTitle></App></root>";
    APP_LIST *apps = NULL;
    assert(xml_applist(catalogue, strlen(catalogue), &apps) == GS_OK);
    assert(apps->id == 9 && apps->uuid[0] == 0 && apps->art_version[0] == 0);
    assert(apps->next->id == 123 && apps->next->hdr == 1);
    assert(strcmp(apps->next->uuid, "9a1c5a25-58fe-40e0-b9aa-7d3f00000005") == 0);
    assert(strcmp(apps->next->art_version, "remote-session-v6") == 0);
    assert(strcmp(apps->next->name, "Remote Monitor") == 0);
    while (apps) { APP_LIST *next = apps->next; free(apps->name); free(apps); apps = next; }
    APP_LIST remote = {0};
    strcpy(remote.uuid, "9a1c5a25-58fe-40e0-b9aa-7d3f00000005");
    assert(xml_app_is_remote_control(&remote) && xml_app_is_remote_display(&remote));
    remote.uuid[35] = '7';
    assert(xml_app_is_remote_control(&remote) && !xml_app_is_remote_display(&remote));
    remote.uuid[0] = '0';
    assert(!xml_app_is_remote_control(&remote));
    STREAM_CONFIGURATION config = {.width=2560, .height=1440, .fps=90,
        .supportedVideoFormats=VIDEO_FORMAT_AV1_MAIN10, .audioConfiguration=AUDIO_CONFIGURATION_STEREO};
    server.unsupported = true;
    server.currentGame = 42;
    reply = "<root status_code=\"200\"><resume>1</resume><sessionUrl0>rtsp://fixture/session</sessionUrl0></root>";
    assert(gs_start_app_ex(&client, &server, &config, 42, false, false, false, 0, NULL,
                          "12345678-1234-1234-1234-123456789abc") == GS_OK);
    assert(strstr(last_url, "/resume?") && strstr(last_url, "appuuid=12345678-1234-1234-1234-123456789abc"));
    free((void *) server.serverInfo.rtspSessionUrl); server.serverInfo.rtspSessionUrl = NULL;
    reply = "<root status_code=\"410\" status_message=\"Remote Monitor disconnected successfully.\"><gamesession>0</gamesession></root>";
    assert(gs_start_app_ex(&client, &server, &config, 43, false, false, false, 0, NULL,
                          "9a1c5a25-58fe-40e0-b9aa-7d3f00000002") == GS_HOST_ACTION_COMPLETE);
    assert(strstr(last_url, "/launch?") && server.currentGame == 42);
    reply = "<root status_message=\"Confirm termination\" status_code=\"410\"><gamesession>0</gamesession></root>";
    assert(gs_start_app_ex(&client, &server, &config, 44, false, false, false, 0, NULL,
                          "9a1c5a25-58fe-40e0-b9aa-7d3f00000004") == GS_ERROR);
    assert(server.currentGame == 42);
    const char *message = NULL;
    gs_get_error(&message);
    assert(strstr(message, "Confirm termination") != NULL);
    puts("VibeShine protocol tests passed");
}
