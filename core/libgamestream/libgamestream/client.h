/*
 * This file is part of Moonlight Embedded.
 *
 * Copyright (C) 2015-2017 Iwan Timmer
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

#pragma once

#include "xml.h"

#include <Limelight.h>

#include <stdbool.h>

#define MIN_SUPPORTED_GFE_VERSION 3
#define MAX_SUPPORTED_GFE_VERSION 7

#define WAKE_METHOD_WOL 0
#define WAKE_METHOD_HTTP 1

typedef struct _SERVER_DATA {
    const char *uuid;
    const char *mac;
    const char *hostname;
    const char *gpuType;
    /** HttpsPort */
    unsigned short httpsPort;
    /** ExternalPort */
    unsigned short extPort;
    bool paired;
    bool supports4K;
    bool supportsHdr;
    bool unsupported;
    bool isGfe;
    int currentGame;
    int serverMajorVersion;
    const char *gsVersion;
    int wake_method;
    char *wake_url;
    PDISPLAY_MODE modes;
    SERVER_INFORMATION serverInfo;
} SERVER_DATA, *PSERVER_DATA;

typedef struct GS_CLIENT_T *GS_CLIENT;

GS_CLIENT gs_new(const char *keydir);

int gs_conf_init(const char *keydir);

void gs_destroy(GS_CLIENT hnd);

void gs_set_timeout(GS_CLIENT hnd, int timeout_secs);

/* Whole-transfer ceiling on every request made through this client (see
 * http_set_total_timeout); 0 = no ceiling (curl default). */
void gs_set_total_timeout(GS_CLIENT hnd, int timeout_secs);

int gs_get_status(GS_CLIENT hnd, PSERVER_DATA server, const char *address, uint16_t port, bool unsupported);

int gs_start_app(GS_CLIENT hnd, PSERVER_DATA server, PSTREAM_CONFIGURATION config, int appId, bool is_gfe, bool sops,
                 bool localaudio, int gamepad_mask, const char *surround_params);

int gs_start_app_ex(GS_CLIENT hnd, PSERVER_DATA server, PSTREAM_CONFIGURATION config, int appId, bool is_gfe,
                    bool sops, bool localaudio, int gamepad_mask, const char *surround_params, const char *app_uuid);

/* Warning returned by the last successful gs_start_app_ex (VibeShine
 * clientWarning/clientWarningMessage). Pointers stay valid until the next launch. */
bool gs_launch_warning(GS_CLIENT hnd, const char **code, const char **message);

int gs_applist(GS_CLIENT hnd, const SERVER_DATA *server, PAPP_LIST *app_list);

int gs_unpair(GS_CLIENT hnd, PSERVER_DATA server);

int gs_pair(GS_CLIENT hnd, PSERVER_DATA server, const char *pin);

int gs_quit_app(GS_CLIENT hnd, PSERVER_DATA server);

int gs_download_cover(GS_CLIENT hnd, const SERVER_DATA *server, int appId, const char *path);

typedef struct {
    bool supported;
    int version;
} GS_ABR_CAPABILITIES;

typedef struct {
    bool enabled;
    int min_bitrate;
    int max_bitrate;
    const char *mode;
} GS_ABR_CONFIG;

typedef struct {
    float packet_loss;
    int rtt_ms;
    float decode_fps;
    int dropped_frames;
    int current_bitrate;
} GS_ABR_FEEDBACK;

typedef struct {
    bool has_new_bitrate;
    int new_bitrate;
} GS_ABR_ACTION;

int gs_set_bitrate(GS_CLIENT hnd, const SERVER_DATA *server, int bitrate_kbps);
/* Returns the bitrate actually accepted by VibeShine, which may clamp it. */
int gs_set_bitrate_ex(GS_CLIENT hnd, const SERVER_DATA *server, int bitrate_kbps, int *applied_kbps);

int gs_get_abr_capabilities(GS_CLIENT hnd, const SERVER_DATA *server, GS_ABR_CAPABILITIES *caps);

/* VibeShine hardware metrics v1 (GET /api/hardware-metrics on the paired HTTPS
 * port; opt-in `hardware_metrics` on the host). Missing readings are -1; only the
 * first listed GPU is read. interval_ms is the host's minimum polling interval. */
typedef struct {
    int interval_ms;
    int cpu_usage_percent;
    int gpu_usage_percent;
    int gpu_encoder_percent;
    int gpu_temperature_c;
    int gpu_vram_used_mb;
} GS_HOST_METRICS;

int gs_parse_hardware_metrics(const char *json, GS_HOST_METRICS *out);
int gs_get_hardware_metrics(GS_CLIENT hnd, const SERVER_DATA *server, GS_HOST_METRICS *out);

int gs_set_abr_mode(GS_CLIENT hnd, const SERVER_DATA *server, const GS_ABR_CONFIG *config);

int gs_report_abr_feedback(GS_CLIENT hnd, const SERVER_DATA *server, const GS_ABR_FEEDBACK *feedback,
                           GS_ABR_ACTION *action);

int gs_http_wake(GS_CLIENT hnd, const char *url);