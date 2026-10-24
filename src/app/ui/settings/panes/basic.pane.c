#include "app.h"
#include "config.h"

#include "pref_obj.h"
#include "pref_fps.h"
#include "pref_res.h"
#include "av_pane.h"
#include "ui/settings/settings.controller.h"

#include "util/i18n.h"
#include "ss4s.h"
#include "ss4s_modules.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Compact stream settings (punktfunk FocusRow style): one labelled control per row.
 * Host, input and application preferences stay in their panes.
 */
typedef struct {
    lv_fragment_t base;
    settings_controller_t *parent;

    lv_obj_t *idr_checkbox;
    lv_obj_t *idr_row;
    lv_obj_t *idr_slider;
    lv_obj_t *idr_value_label;
    int idr_refresh_slider_value;
    lv_obj_t *abr_dropdown;
    pref_dropdown_int_entry_t abr_entries[3];
    lv_obj_t *full_range_checkbox;
    lv_obj_t *sliced_checkbox;
    lv_obj_t *bitrate_label;
    lv_obj_t *bitrate_slider;
    lv_obj_t *hevc_checkbox;
    lv_obj_t *av1_checkbox;
    lv_obj_t *hdr_checkbox;
    lv_obj_t *force_10bit_checkbox;

    pref_dropdown_string_entry_t *vdec_entries;
    int vdec_entries_len;

    pref_dropdown_int_entry_t surround_entries[3];
    int surround_entries_len;
} basic_pane_t;

static void pane_ctor(lv_fragment_t *self, void *args);

static void pane_dtor(lv_fragment_t *self);

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container);

static void on_bitrate_changed(lv_event_t *e);

static void on_res_fps_updated(lv_event_t *e);

static void update_bitrate_label(basic_pane_t *pane);

static void hdr_state_update(basic_pane_t *pane);

static void hdr_state_update_cb(lv_event_t *e);

static void codec_checkbox_cb(lv_event_t *e);

static void module_changed_cb(lv_event_t *e);

static void abr_state_update(basic_pane_t *pane);

static void abr_checkbox_cb(lv_event_t *e);

static void idr_refresh_state_update(basic_pane_t *pane);

static void idr_checkbox_activate(lv_event_t *e);

static void idr_refresh_checkbox_cb(lv_event_t *e);

static void idr_refresh_slider_cb(lv_event_t *e);

static void reconnect_cb(lv_event_t *e);

const lv_fragment_class_t settings_pane_basic_cls = {
    .constructor_cb = pane_ctor,
    .destructor_cb = pane_dtor,
    .create_obj_cb = create_obj,
    .instance_size = sizeof(basic_pane_t),
};

#define BITRATE_STEP 1000

static void pane_ctor(lv_fragment_t *self, void *args) {
    basic_pane_t *pane = (basic_pane_t *) self;
    pane->parent = args;
    pane->abr_entries[0] = (pref_dropdown_int_entry_t) {locstr("Balanced"), 0, true};
    pane->abr_entries[1] = (pref_dropdown_int_entry_t) {locstr("Quality"), 1, false};
    pane->abr_entries[2] = (pref_dropdown_int_entry_t) {locstr("Low latency"), 2, false};

    app_t *app = pane->parent->app;

    array_list_t modules = app->ss4s.modules;
    pane->vdec_entries = calloc(modules.size + 1, sizeof(pref_dropdown_string_entry_t));
    set_decoder_entry(&pane->vdec_entries[pane->vdec_entries_len++], locstr("Auto"), "auto", true);
    for (int module_idx = 0; module_idx < modules.size; module_idx++) {
        const SS4S_ModuleInfo *info = array_list_get(&modules, module_idx);
        const char *group = SS4S_ModuleInfoGetGroup(info);
        if (info->has_video && !contains_decoder_group(pane->vdec_entries, pane->vdec_entries_len, group)) {
            set_decoder_entry(&pane->vdec_entries[pane->vdec_entries_len++], info->name, group, false);
        }
    }

    unsigned int supported_ch = app->ss4s.audio_cap.maxChannels;
    if (supported_ch < 6) {
        supported_ch = 8;
    }
    for (int i = 0; i < (int) audio_config_len; i++) {
        audio_config_entry_t config = audio_configs[i];
        if (supported_ch < CHANNEL_COUNT_FROM_AUDIO_CONFIGURATION(config.configuration)) {
            continue;
        }
        pref_dropdown_int_entry_t *entry = &pane->surround_entries[pane->surround_entries_len];
        entry->name = locstr(config.name);
        entry->value = config.configuration;
        entry->fallback = config.configuration == AUDIO_CONFIGURATION_STEREO;
        pane->surround_entries_len++;
    }
}

static void pane_dtor(lv_fragment_t *self) {
    basic_pane_t *pane = (basic_pane_t *) self;
    free(pane->vdec_entries);
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container) {
    basic_pane_t *pane = (basic_pane_t *) self;
    settings_controller_t *parent = pane->parent;
    app_t *app = parent->app;
    lv_obj_t *view = pref_pane_container(container);
    lv_obj_set_layout(view, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(view, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(view, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    int max_width = (int) app->ss4s.video_cap.maxWidth, max_height = (int) app->ss4s.video_cap.maxHeight;
    int native_width = app->ui.width, native_height = app->ui.height;
#if TARGET_WEBOS
    if (parent->panel_width > 0 && parent->panel_height > 0) {
        native_width = parent->panel_width;
        native_height = parent->panel_height;
    }
#endif
    if (max_width == 0 || max_height == 0) {
        max_width = native_width;
        max_height = native_height;
    }

    lv_obj_t *res_row = pref_focus_row(view, locstr("Resolution"));
    lv_obj_t *res_dropdown = pref_dropdown_res(res_row, native_width, native_height, max_width, max_height,
                                               &app_configuration->stream.width, &app_configuration->stream.height);
    pref_style_inline_control(res_dropdown);
    pref_row_bind_control(res_row, res_dropdown);
    lv_obj_add_event_cb(res_dropdown, on_res_fps_updated, LV_EVENT_VALUE_CHANGED, self);

    unsigned int max_fps = app->ss4s.video_cap.maxFps;
#if TARGET_WEBOS
    if (parent->panel_fps > 0 && (max_fps == 0 || parent->panel_fps < max_fps)) {
        max_fps = parent->panel_fps;
    }
#endif
    const static int fps_options[] = {30, 60, 90, 120, 144, 240, 0};
    lv_obj_t *fps_row = pref_focus_row(view, locstr("Frame rate"));
    lv_obj_t *fps_dropdown = pref_dropdown_fps(fps_row, fps_options, (int) max_fps, &app_configuration->stream.fps,
                                               &app_configuration->client_refresh_rate_x100);
    pref_style_inline_control(fps_dropdown);
    pref_row_bind_control(fps_row, fps_dropdown);
    lv_obj_add_event_cb(fps_dropdown, on_res_fps_updated, LV_EVENT_VALUE_CHANGED, self);

    lv_obj_t *br_row = pref_focus_row(view, locstr("Bitrate"));
    pane->bitrate_label = lv_label_create(br_row);
    lv_obj_set_width(pane->bitrate_label, LV_PCT(18));
    lv_obj_set_style_max_width(pane->bitrate_label, LV_DPX(96), 0);
    lv_obj_set_style_text_align(pane->bitrate_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_clear_flag(pane->bitrate_label, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    unsigned int max = 400000;
    lv_obj_t *bitrate_slider = pref_slider(br_row, &app_configuration->stream.bitrate, 5000, (int) max, BITRATE_STEP);
    lv_obj_set_width(bitrate_slider, LV_PCT(32));
    lv_obj_set_style_max_width(bitrate_slider, LV_DPX(188), 0);
    lv_obj_clear_flag(bitrate_slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    lv_obj_add_event_cb(bitrate_slider, on_bitrate_changed, LV_EVENT_VALUE_CHANGED, self);
    pane->bitrate_slider = bitrate_slider;
    pref_row_bind_control(br_row, bitrate_slider);
    update_bitrate_label(pane);

    lv_obj_t *vdec_row = pref_focus_row(view, locstr("Video backend"));
    lv_obj_t *vdec_dropdown = pref_dropdown_string(vdec_row, pane->vdec_entries, pane->vdec_entries_len,
                                                   &app_configuration->decoder);
    pref_style_inline_control(vdec_dropdown);
    pref_row_bind_control(vdec_row, vdec_dropdown);
    lv_obj_add_event_cb(vdec_dropdown, module_changed_cb, LV_EVENT_VALUE_CHANGED, pane);

    lv_obj_t *hevc_checkbox = pref_checkbox(view, locstr("HEVC"), &app_configuration->hevc, false);
    pane->hevc_checkbox = hevc_checkbox;
    lv_obj_add_event_cb(hevc_checkbox, codec_checkbox_cb, LV_EVENT_VALUE_CHANGED, pane);

    lv_obj_t *av1_checkbox = pref_checkbox(view, locstr("AV1"), &app_configuration->av1, false);
    pane->av1_checkbox = av1_checkbox;
    if (app->ss4s.video_cap.codecs & SS4S_VIDEO_AV1) {
        lv_obj_clear_state(av1_checkbox, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(av1_checkbox, LV_STATE_DISABLED);
    }
    lv_obj_add_event_cb(av1_checkbox, codec_checkbox_cb, LV_EVENT_VALUE_CHANGED, pane);

    pane->hdr_checkbox = pref_checkbox(view, locstr("HDR"), &app_configuration->hdr, false);
    lv_obj_add_event_cb(pane->hdr_checkbox, hdr_state_update_cb, LV_EVENT_VALUE_CHANGED, pane);

    pane->force_10bit_checkbox = pref_checkbox(view, locstr("10-bit SDR"), &app_configuration->force_10bit, false);
    lv_obj_add_event_cb(pane->force_10bit_checkbox, hdr_state_update_cb, LV_EVENT_VALUE_CHANGED, pane);

    lv_obj_t *audio_row = pref_focus_row(view, locstr("Audio"));
    lv_obj_t *ch_dropdown = pref_dropdown_int(audio_row, pane->surround_entries, pane->surround_entries_len,
                                              &app_configuration->stream.audioConfiguration, NULL);
    pref_style_inline_control(ch_dropdown);
    pref_row_bind_control(audio_row, ch_dropdown);
    lv_obj_add_event_cb(ch_dropdown, module_changed_cb, LV_EVENT_VALUE_CHANGED, pane);

    pref_header(view, locstr("Video"));

    lv_obj_t *full_range = pref_checkbox(view, locstr("Full range YUV (SDR only)"),
                                         &app_configuration->force_full_color_range, false);
    pane->full_range_checkbox = full_range;
    lv_obj_add_event_cb(full_range, reconnect_cb, LV_EVENT_VALUE_CHANGED, pane);

    lv_obj_t *sliced = pref_checkbox(view, locstr("HEVC sliced frames"),
                                     &app_configuration->hevc_sliced_frames, false);
    pane->sliced_checkbox = sliced;
    lv_obj_add_event_cb(sliced, reconnect_cb, LV_EVENT_VALUE_CHANGED, pane);

    lv_obj_t *idr_checkbox = lv_checkbox_create(view);
    lv_checkbox_set_text(idr_checkbox, locstr("Periodic decoder refresh (HEVC)"));
    lv_obj_set_size(idr_checkbox, LV_PCT(100), PREF_ROW_HEIGHT);
    lv_obj_set_style_pad_hor(idr_checkbox, LV_DPX(16), 0);
    lv_obj_set_style_pad_top(idr_checkbox, LV_DPX(12), 0);
    lv_obj_set_style_pad_bottom(idr_checkbox, LV_DPX(8), 0);
    lv_obj_set_style_radius(idr_checkbox, LV_DPX(8), 0);
    if (app_configuration->idr_refresh_interval_ms >= 500) {
        lv_obj_add_state(idr_checkbox, LV_STATE_CHECKED);
    }
    pane->idr_checkbox = idr_checkbox;
    pane->idr_refresh_slider_value = app_configuration->idr_refresh_interval_ms >= 500
            ? app_configuration->idr_refresh_interval_ms
            : 10000;
    pref_checkbox_prepare_for_dpad(idr_checkbox);
    pane->idr_row = pref_focus_row(view, locstr("Refresh interval"));
    pane->idr_value_label = lv_obj_get_child(pane->idr_row, 0);
    lv_obj_t *idr_slider = pref_slider(pane->idr_row, &pane->idr_refresh_slider_value, 500, 60000, 500);
    pref_style_inline_control(idr_slider);
    pref_row_bind_control(pane->idr_row, idr_slider);
    pane->idr_slider = idr_slider;
    lv_obj_add_event_cb(idr_checkbox, idr_checkbox_activate, LV_EVENT_CLICKED, pane);
    lv_obj_add_event_cb(idr_slider, idr_refresh_slider_cb, LV_EVENT_VALUE_CHANGED, pane);
    idr_refresh_state_update(pane);

    pref_header(view, locstr("Bitrate"));

    lv_obj_t *abr_checkbox = pref_checkbox(view, locstr("Adaptive bitrate"),
                                           &app_configuration->auto_adjust_bitrate, false);
    lv_obj_t *abr_row = pref_focus_row(view, locstr("Profile"));
    pane->abr_dropdown = pref_dropdown_int(abr_row, pane->abr_entries,
                                           sizeof(pane->abr_entries) / sizeof(pane->abr_entries[0]),
                                           &app_configuration->abr_mode, NULL);
    pref_style_inline_control(pane->abr_dropdown);
    pref_row_bind_control(abr_row, pane->abr_dropdown);
    lv_obj_add_event_cb(abr_checkbox, abr_checkbox_cb, LV_EVENT_VALUE_CHANGED, pane);
    lv_obj_add_event_cb(pane->abr_dropdown, reconnect_cb, LV_EVENT_VALUE_CHANGED, pane);
    abr_state_update(pane);

    hdr_state_update(pane);
    return view;
}

static void on_bitrate_changed(lv_event_t *e) {
    reconnect_cb(e);
    update_bitrate_label(lv_event_get_user_data(e));
}

static void on_res_fps_updated(lv_event_t *e) {
    basic_pane_t *pane = lv_event_get_user_data(e);
    pane->parent->needs_stream_reconnect = true;
}

static void update_bitrate_label(basic_pane_t *pane) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d Mbps", app_configuration->stream.bitrate / 1000);
    lv_label_set_text(pane->bitrate_label, buf);
}

static void hdr_state_update(basic_pane_t *pane) {
    app_t *app = pane->parent->app;
    const bool hevc_hdr = app_configuration->hevc && (app->ss4s.video_cap.codecs & SS4S_VIDEO_H265);
    const bool av1_hdr = app_configuration->av1 && (app->ss4s.video_cap.codecs & SS4S_VIDEO_AV1);
    const bool main10_codec = hevc_hdr || av1_hdr;
    bool hdr_ok = main10_codec && app->ss4s.video_cap.hdr;
    if (pane->sliced_checkbox != NULL) {
        if (hevc_hdr && !av1_hdr) lv_obj_clear_state(pane->sliced_checkbox, LV_STATE_DISABLED);
        else lv_obj_add_state(pane->sliced_checkbox, LV_STATE_DISABLED);
        idr_refresh_state_update(pane);
        if (hdr_ok && app_configuration->hdr) lv_obj_add_state(pane->full_range_checkbox, LV_STATE_DISABLED);
        else lv_obj_clear_state(pane->full_range_checkbox, LV_STATE_DISABLED);
    }

    if (hdr_ok) {
        lv_obj_clear_state(pane->hdr_checkbox, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(pane->hdr_checkbox, LV_STATE_DISABLED);
    }
    if (main10_codec) {
        lv_obj_clear_state(pane->force_10bit_checkbox, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(pane->force_10bit_checkbox, LV_STATE_DISABLED);
    }
}

static void hdr_state_update_cb(lv_event_t *e) {
    reconnect_cb(e);
    hdr_state_update(lv_event_get_user_data(e));
}

static void codec_checkbox_cb(lv_event_t *e) {
    basic_pane_t *pane = lv_event_get_user_data(e);
    lv_obj_t *target = lv_event_get_target(e);
    /* Selecting AV1 must not leave HEVC advertised (host would pick HEVC). */
    if (target == pane->av1_checkbox && app_configuration->av1 && app_configuration->hevc) {
        app_configuration->hevc = false;
        lv_obj_clear_state(pane->hevc_checkbox, LV_STATE_CHECKED);
    } else if (target == pane->hevc_checkbox && app_configuration->hevc && app_configuration->av1) {
        app_configuration->av1 = false;
        lv_obj_clear_state(pane->av1_checkbox, LV_STATE_CHECKED);
    }
    pane->parent->needs_stream_reconnect = true;
    hdr_state_update(pane);
}

static void module_changed_cb(lv_event_t *e) {
    basic_pane_t *pane = lv_event_get_user_data(e);
    settings_controller_t *parent = pane->parent;
    parent->needs_stream_reconnect = true;
    hdr_state_update(pane);
}

static void reconnect_cb(lv_event_t *e) {
    basic_pane_t *pane = lv_event_get_user_data(e);
    if (pane->parent) {
        pane->parent->needs_stream_reconnect = true;
    }
}

static void abr_state_update(basic_pane_t *pane) {
    if (app_configuration->auto_adjust_bitrate) {
        lv_obj_clear_state(pane->abr_dropdown, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(pane->abr_dropdown, LV_STATE_DISABLED);
    }
}

static void abr_checkbox_cb(lv_event_t *e) {
    basic_pane_t *pane = lv_event_get_user_data(e);
    reconnect_cb(e);
    abr_state_update(pane);
}

static void idr_refresh_state_update(basic_pane_t *pane) {
    const bool hevc_on = app_configuration->hevc && !app_configuration->av1 &&
            (pane->parent->app->ss4s.video_cap.codecs & SS4S_VIDEO_H265);
    const bool refresh_on = app_configuration->idr_refresh_interval_ms >= 500;
    if (refresh_on) {
        lv_obj_add_state(pane->idr_checkbox, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(pane->idr_checkbox, LV_STATE_CHECKED);
    }
    if (!hevc_on) {
        lv_obj_add_state(pane->idr_checkbox, LV_STATE_DISABLED);
        lv_obj_add_state(pane->idr_row, LV_STATE_DISABLED);
        lv_obj_add_state(pane->idr_slider, LV_STATE_DISABLED);
    } else {
        lv_obj_clear_state(pane->idr_checkbox, LV_STATE_DISABLED);
        if (refresh_on) {
            lv_obj_clear_state(pane->idr_row, LV_STATE_DISABLED);
            lv_obj_clear_state(pane->idr_slider, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(pane->idr_row, LV_STATE_DISABLED);
            lv_obj_add_state(pane->idr_slider, LV_STATE_DISABLED);
        }
    }
    if (refresh_on) {
        lv_label_set_text_fmt(pane->idr_value_label, "%s - %.1f s",
                              locstr("Refresh interval"),
                              app_configuration->idr_refresh_interval_ms / 1000.0);
    } else {
        lv_label_set_text(pane->idr_value_label, locstr("Refresh interval"));
    }
}

static void idr_checkbox_activate(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    lv_obj_t *cb = lv_event_get_current_target(e);
    if (lv_obj_has_state(cb, LV_STATE_CHECKED)) {
        lv_obj_clear_state(cb, LV_STATE_CHECKED);
    } else {
        lv_obj_add_state(cb, LV_STATE_CHECKED);
    }
    idr_refresh_checkbox_cb(e);
}

static void idr_refresh_checkbox_cb(lv_event_t *e) {
    basic_pane_t *pane = lv_event_get_user_data(e);
    lv_obj_t *cb = lv_event_get_current_target(e);
    if (lv_obj_has_state(cb, LV_STATE_CHECKED)) {
        if (pane->idr_refresh_slider_value < 500) {
            pane->idr_refresh_slider_value = 10000;
        }
        app_configuration->idr_refresh_interval_ms = pane->idr_refresh_slider_value;
        if (pane->parent) {
            pane->parent->needs_stream_reconnect = true;
        }
    } else {
        app_configuration->idr_refresh_interval_ms = 0;
    }
    reconnect_cb(e);
    idr_refresh_state_update(pane);
}

static void idr_refresh_slider_cb(lv_event_t *e) {
    basic_pane_t *pane = lv_event_get_user_data(e);
    if (app_configuration->idr_refresh_interval_ms >= 500) {
        app_configuration->idr_refresh_interval_ms = pane->idr_refresh_slider_value;
        if (pane->parent) {
            pane->parent->needs_stream_reconnect = true;
        }
    }
    idr_refresh_state_update(pane);
}
