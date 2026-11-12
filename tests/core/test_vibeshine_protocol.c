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
#include <locale.h>
#include <math.h>

static const char *reply;
static int transport_status;
static char last_url[4096];
static char last_payload[256];
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
int http_post_json(HTTP *http, char *url, const char *payload, HTTP_DATA *data) {
    (void)http; (void)url;
    snprintf(last_payload, sizeof(last_payload), "%s", payload);
    data->memory = strdup("{\"newBitrate\":45000}");
    data->size = strlen(data->memory);
    return GS_OK;
}

bool uuidstr_random(uuidstr_t *dest) {
    memcpy(dest, "00000000-0000-4000-8000-000000000000", 37);
    return true;
}

int main(void) {
    struct GS_CLIENT_T client = {0};
    SERVER_DATA server = {.serverInfo.address = "fixture", .currentGame = 42};
    GS_ABR_FEEDBACK feedback = {.packet_loss = 2.25f, .decode_fps = 59.9f,
        .rtt_ms = 10, .current_bitrate = 50000};
    GS_ABR_ACTION action;
    const char *comma_locale = setlocale(LC_NUMERIC, "pt_BR.UTF-8");
    if (!comma_locale) puts("Comma numeric locale unavailable; checking C locale only");
    assert(gs_report_abr_feedback(&client, &server, &feedback, &action) == GS_OK);
    assert(strstr(last_payload, "\"packetLoss\":2.25") != NULL);
    assert(strstr(last_payload, "\"decodeFps\":59.9") != NULL);
    assert(action.has_new_bitrate && action.new_bitrate == 45000);
    feedback.packet_loss = NAN;
    assert(gs_report_abr_feedback(&client, &server, &feedback, &action) == GS_INVALID);
    feedback.packet_loss = 0;
    feedback.decode_fps = INFINITY;
    assert(gs_report_abr_feedback(&client, &server, &feedback, &action) == GS_INVALID);
    setlocale(LC_NUMERIC, "C");
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
    /* Host warnings: read through a successful launch, never block it. */
    const char *wcode = NULL, *wmsg = NULL;
    assert(!gs_launch_warning(&client, &wcode, &wmsg)); /* a failed launch clears it */
    server.currentGame = 0;
    reply = "<root status_code=\"200\"><gamesession>1</gamesession><sessionUrl0>rtsp://fixture/s</sessionUrl0>"
            "<clientWarning>virtual_display_disabled_by_client</clientWarning></root>";
    assert(gs_start_app_ex(&client, &server, &config, 9, false, false, false, 0, NULL, NULL) == GS_OK);
    assert(gs_launch_warning(&client, &wcode, &wmsg));
    assert(strcmp(wcode, "virtual_display_disabled_by_client") == 0 && wmsg == NULL);
    free((void *) server.serverInfo.rtspSessionUrl); server.serverInfo.rtspSessionUrl = NULL;
    reply = "<root status_code=\"200\"><gamesession>1</gamesession><sessionUrl0>rtsp://fixture/s</sessionUrl0>"
            "<clientWarning>new_code</clientWarning>"
            "<clientWarningMessage>  Line 1\nLine 2\r\n\xe2\x80\xae" "rev\tok\n4\n5  </clientWarningMessage></root>";
    assert(gs_start_app_ex(&client, &server, &config, 9, false, false, false, 0, NULL, NULL) == GS_OK);
    assert(gs_launch_warning(&client, &wcode, &wmsg));
    assert(strcmp(wcode, "new_code") == 0);
    assert(strcmp(wmsg, "Line 1\nLine 2\nrev ok\n4 5") == 0); /* XML folds CRLF; bidi dropped; 3 breaks max */
    free((void *) server.serverInfo.rtspSessionUrl); server.serverInfo.rtspSessionUrl = NULL;
    /* Next launch without a warning clears the previous one. */
    reply = "<root status_code=\"200\"><gamesession>1</gamesession></root>";
    assert(gs_start_app_ex(&client, &server, &config, 9, false, false, false, 0, NULL, NULL) == GS_OK);
    assert(!gs_launch_warning(&client, &wcode, &wmsg));
    char wbuf_code[GS_LAUNCH_WARNING_CODE_MAX + 1], wbuf_msg[GS_LAUNCH_WARNING_MESSAGE_MAX + 1];
    const char *ignored[] = {
        /* message without code, duplicate code, not started, bad status, bad code chars */
        "<root status_code=\"200\"><gamesession>1</gamesession><clientWarningMessage>x</clientWarningMessage></root>",
        "<root status_code=\"200\"><gamesession>1</gamesession><clientWarning>a</clientWarning><clientWarning>b</clientWarning></root>",
        "<root status_code=\"200\"><gamesession>0</gamesession><clientWarning>a</clientWarning></root>",
        "<root status_code=\"500\"><gamesession>1</gamesession><clientWarning>a</clientWarning></root>",
        "<root status_code=\"200\"><gamesession>1</gamesession><clientWarning>a b</clientWarning></root>",
        "<root status_code=\"200\"><gamesession>1</gamesession><clientWarning>a</clientWarning>",
    };
    for (size_t i = 0; i < sizeof(ignored) / sizeof(*ignored); i++) {
        assert(!xml_launch_warning(ignored[i], strlen(ignored[i]), wbuf_code, sizeof(wbuf_code), wbuf_msg,
                                   sizeof(wbuf_msg)));
        assert(wbuf_code[0] == 0 && wbuf_msg[0] == 0);
    }
    /* A duplicate message discards the warning; an oversized message is dropped. */
    const char *dup = "<root status_code=\"200\"><resume>1</resume><clientWarning>a</clientWarning>"
                      "<clientWarningMessage>x</clientWarningMessage><clientWarningMessage>y</clientWarningMessage></root>";
    assert(!xml_launch_warning(dup, strlen(dup), wbuf_code, sizeof(wbuf_code), wbuf_msg, sizeof(wbuf_msg)));
    char big[1400];
    int off = snprintf(big, sizeof(big), "<root status_code=\"200\"><resume>1</resume><clientWarning>a</clientWarning>"
                                         "<clientWarningMessage>");
    memset(big + off, 'x', 600);
    snprintf(big + off + 600, sizeof(big) - off - 600, "</clientWarningMessage></root>");
    assert(xml_launch_warning(big, strlen(big), wbuf_code, sizeof(wbuf_code), wbuf_msg, sizeof(wbuf_msg)));
    assert(strcmp(wbuf_code, "a") == 0 && wbuf_msg[0] == 0);
    /* Hardware metrics v1: first GPU only, missing readings -1, '.' decimals in any locale. */
    GS_HOST_METRICS hm;
    setlocale(LC_NUMERIC, comma_locale ? "pt_BR.UTF-8" : "C");
    assert(gs_parse_hardware_metrics("{\"version\":1,\"sampled_at_unix_ms\":1,\"interval_ms\":1000,"
                                     "\"cpu\":{\"cpu_usage_percent\":34.6},\"gpus\":[{\"id\":-5,\"name\":\"RTX\","
                                     "\"metrics\":{\"gpu_usage_percent\":80,\"gpu_encoder_percent\":45.2,"
                                     "\"gpu_temperature_c\":71,\"gpu_vram_used_mb\":5120}},"
                                     "{\"id\":2,\"metrics\":{\"gpu_usage_percent\":3}}]}", &hm) == GS_OK);
    setlocale(LC_NUMERIC, "C");
    assert(hm.interval_ms == 1000 && hm.cpu_usage_percent == 35 && hm.gpu_usage_percent == 80);
    assert(hm.gpu_encoder_percent == 45 && hm.gpu_temperature_c == 71 && hm.gpu_vram_used_mb == 5120);
    assert(gs_parse_hardware_metrics("{\"version\":1,\"interval_ms\":1000,\"cpu\":{},\"gpus\":[]}", &hm) == GS_OK);
    assert(hm.cpu_usage_percent == -1 && hm.gpu_usage_percent == -1 && hm.gpu_temperature_c == -1);
    assert(gs_parse_hardware_metrics("{\"version\":2,\"cpu\":{\"cpu_usage_percent\":5}}", &hm) != GS_OK);
    assert(gs_parse_hardware_metrics("{\"error\":\"Hardware metrics are disabled\"}", &hm) != GS_OK);
    puts("VibeShine protocol tests passed");
}
