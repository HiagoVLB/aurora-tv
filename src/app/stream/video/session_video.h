#pragma once

#include <Limelight.h>

extern struct VIDEO_STATS vdec_summary_stats;
extern struct VIDEO_INFO vdec_stream_info;
extern struct AUDIO_INFO audio_stream_info;

extern DECODER_RENDERER_CALLBACKS ss4s_dec_callbacks;

/** Tear-free copy of vdec_summary_stats for cross-thread readers (seqlock retry). */
void vdec_stats_snapshot(struct VIDEO_STATS *out);

struct session_config_t;
/** Call before LiStartConnection with the effective session configuration. */
void session_video_prepare_stream(const struct session_config_t *config);


/** Periodic file diagnostics, called on the UI thread even with overlay off. */
void session_video_log_diagnostics(void);
