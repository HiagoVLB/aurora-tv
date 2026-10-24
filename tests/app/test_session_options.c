#include "../../src/app/stream/session.c"
#include "stream/session.h"
#include "stream/video/session_video.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    app_t app = {0};
    app.ss4s.video_cap.codecs = SS4S_VIDEO_H264 | SS4S_VIDEO_H265 | SS4S_VIDEO_AV1;
    app.ss4s.video_cap.hdr = true;
    app.ss4s.video_cap.maxBitrate = 90000;
    app_settings_t settings;
    settings_initialize(&settings, "/tmp/aurora-session-options");
    settings.hevc = true;
    settings.av1 = false;
    settings.hdr = false;
    settings.force_full_color_range = true;
    settings.hevc_sliced_frames = true;
    settings.stream.width = 2560; settings.stream.height = 1440; settings.stream.fps = 90;
    settings.idr_refresh_interval_ms = 10000;
    settings.auto_adjust_bitrate = true;
    settings.abr_mode = 2;
    session_config_t config = {0};
    session_config_init(&app, &config, NULL, &settings);
    assert(config.stream.bitrate == 90000);
    assert(config.stream.colorRange == COLOR_RANGE_FULL);
    assert(config.auto_adjust_bitrate && config.abr_mode == 2);
    assert(config.hevc_sliced_frames && config.idr_refresh_interval_ms == 10000);
    session_video_prepare_stream(&config);
    assert(ss4s_dec_callbacks.capabilities == (CAPABILITY_DIRECT_SUBMIT |
        CAPABILITY_REFERENCE_FRAME_INVALIDATION_HEVC | CAPABILITY_SLICES_PER_FRAME(6)));
    settings.idr_refresh_interval_ms = 0;
    settings.hevc_sliced_frames = false;
    settings.game_mode = false;
    assert(config.idr_refresh_interval_ms == 10000 && config.hevc_sliced_frames && config.game_mode);
    settings.hdr = true;
    config = (session_config_t){0};
    session_config_init(&app, &config, NULL, &settings);
    assert(config.stream.colorRange == COLOR_RANGE_LIMITED);
    settings.av1 = true;
    config = (session_config_t){0};
    session_config_init(&app, &config, NULL, &settings);
    assert((config.stream.supportedVideoFormats & VIDEO_FORMAT_MASK_H265) == 0);
    session_video_prepare_stream(&config);
    assert(ss4s_dec_callbacks.capabilities == CAPABILITY_DIRECT_SUBMIT);
    settings_clear(&settings);
    puts("Session option snapshots, HEVC slices, AV1 fallback and SDR/HDR range checks passed");
    return 0;
}
