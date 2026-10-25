#include "app.h"
#include "help.dialog.h"

#include "util/i18n.h"
#include "os_info.h"
#include "lvgl/theme/lv_theme_moonlight_colors.h"

#include <stdlib.h>
#include <stdio.h>

#if TARGET_WEBOS
#include <SDL.h>
#include <stdbool.h>
extern bool SDL_webOSGetPanelResolution(int *width, int *height);
extern bool SDL_webOSGetRefreshRate(int *rate);
#endif

static void menu_cancel_cb(lv_event_t *e);

static void help_dialog_resize_cb(lv_event_t *e);

static lv_obj_t *add_help_text(lv_obj_t *list, const char *text);

static void add_section(lv_obj_t *list, const char *title);

static void add_shortcut(lv_obj_t *list, const char *keys, const char *action);

static void add_about_line(lv_obj_t *list, const char *title, const char *value);

static const char *module_id(const SS4S_ModuleInfo *info);

lv_obj_t *help_dialog_create(app_t *app) {
    lv_obj_t *msgbox = lv_msgbox_create(NULL, locstr("Controls & shortcuts"), NULL, NULL, true);
    lv_obj_clear_flag(msgbox, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(lv_obj_get_parent(msgbox), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(msgbox, help_dialog_resize_cb, LV_EVENT_SIZE_CHANGED, NULL);
    lv_obj_set_size(msgbox, LV_PCT(80), LV_PCT(90));
    lv_obj_set_style_max_width(msgbox, LV_PCT(90), 0);

    lv_obj_t *close = lv_msgbox_get_close_btn(msgbox);
    lv_group_t *group = lv_obj_get_group(close);
    lv_group_remove_obj(close);

    lv_obj_t *content = lv_msgbox_get_content(msgbox);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(content, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_t *list = lv_list_create(content);
    lv_obj_add_event_cb(list, menu_cancel_cb, LV_EVENT_CANCEL, msgbox);
    lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
    lv_obj_add_flag(list, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLL_WITH_ARROW);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_style_border_side(list, LV_BORDER_SIDE_NONE, 0);
    lv_obj_set_style_radius(list, 0, 0);
    lv_obj_set_style_pad_all(list, LV_DPX(8), 0);
    lv_obj_set_style_pad_row(list, LV_DPX(6), 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);

    add_help_text(list, locstr("During streaming, use these shortcuts to control Aurora. Scroll with Up/Down or the mouse wheel. Press Back/B/Escape to close this guide."));

    add_section(list, locstr("TV remote"));
    add_shortcut(list, locstr("Hold Back / press Red or Exit"),
                 locstr("Open the stream menu. A short Back press sends Escape to the PC; holding Back on a Magic Remote sends Exit."));
    add_shortcut(list, locstr("Blue"), locstr("Open the on-screen keyboard. Press Back to close it."));
    add_shortcut(list, locstr("Hold OK (0.6 s), then release"), locstr("Right-click on the PC. A short OK press sends Enter."));
    add_shortcut(list, locstr("Channel Up / Down"), locstr("Send Page Up / Page Down to the PC."));
    add_shortcut(list, "Home", locstr("Send the Windows key to the PC (Start menu)."));
    add_shortcut(list, "0–9", locstr("Send Numpad 0–9 to the PC."));
    add_shortcut(list, locstr("Yellow"), locstr("Cycle the diagnostic log overlay: Off, Live, Frozen."));

    add_section(list, locstr("Gamepad"));
    add_shortcut(list, locstr("LB + RB + Back + Start, then release all"), locstr("Open the stream menu."));
    add_shortcut(list, locstr("Hold Select / Back (4 s)"),
                 locstr("Show or hide pinned performance stats. Hold this button alone; it is also sent to the PC."));
    add_shortcut(list, locstr("Y / Triangle with virtual mouse enabled"), locstr("Open the on-screen keyboard."));

    add_section(list, locstr("Virtual mouse"));
    add_help_text(list, locstr("Enable Virtual mouse in the stream menu, or enable it in Settings > Input to start each stream with it."));
    add_shortcut(list, locstr("Right stick / Left stick"), locstr("Move the pointer / scroll."));
    add_shortcut(list, locstr("LT / RT"), locstr("Left-click / right-click."));

    add_section(list, locstr("On-screen keyboard"));
    add_shortcut(list, locstr("D-pad + A / OK"), locstr("Move between keys and press the selected key."));
    add_shortcut(list, locstr("Y / X / Start"), locstr("Space / Backspace / Enter (standard gamepad layout)."));
    add_shortcut(list, locstr("LT / LB / RB"), locstr("Switch letters and symbols / move the text cursor left / right."));
    add_shortcut(list, locstr("B / Back"), locstr("Close the keyboard. Gamepad B also sends Escape to the PC."));

    add_section(list, locstr("Leaving a stream"));
    add_shortcut(list, locstr("Disconnect"), locstr("Return to Aurora and leave the application running on the PC so you can resume later."));
    add_shortcut(list, locstr("End session"), locstr("Ask the host to close the application after confirmation. Not available for Remote Monitor / Remote Input streams."));

    add_section(list, locstr("About Aurora"));
    if (app != NULL) {
        add_about_line(list, locstr("Product"), locstr("Aurora - Moonlight streaming client for LG webOS"));
        add_about_line(list, locstr("Version"), APP_VERSION);
        add_about_line(list, locstr("Video decoder"), module_id(app->ss4s.selection.video_module));
        add_about_line(list, locstr("Audio backend"), module_id(app->ss4s.selection.audio_module));
        char *os_str = os_info_str(&app->os_info);
        if (os_str != NULL) {
            add_about_line(list, locstr("System"), os_str);
            free(os_str);
        }
#if TARGET_WEBOS
        int panel_w = 0, panel_h = 0;
        if (SDL_webOSGetPanelResolution(&panel_w, &panel_h) && panel_w > 0 && panel_h > 0) {
            char resolution_text[32];
            snprintf(resolution_text, sizeof(resolution_text), "%d × %d", panel_w, panel_h);
            add_about_line(list, locstr("Screen resolution"), resolution_text);
        }
        int panel_fps = 0;
        if (SDL_webOSGetRefreshRate(&panel_fps) && panel_fps > 0) {
            char fps_text[32];
            snprintf(fps_text, sizeof(fps_text), "%d Hz", panel_fps);
            add_about_line(list, locstr("Refresh rate"), fps_text);
        }
#endif
    }

    if (group != NULL) {
        lv_group_add_obj(group, list);
        lv_group_focus_obj(list);
    }
    lv_obj_center(msgbox);
    return msgbox;
}

static void menu_cancel_cb(lv_event_t *e) {
    lv_obj_t *target = lv_event_get_target(e);
    if (target != lv_event_get_current_target(e)) return;
    lv_msgbox_close(lv_event_get_user_data(e));
}

static void help_dialog_resize_cb(lv_event_t *e) {
    lv_obj_t *msgbox = lv_event_get_current_target(e);
    lv_obj_t *title = lv_msgbox_get_title(msgbox);
    lv_obj_t *close_btn = lv_msgbox_get_close_btn(msgbox);
    lv_coord_t title_height = LV_MAX(lv_obj_get_height(title), lv_obj_get_height(close_btn));
    lv_obj_t *content = lv_msgbox_get_content(msgbox);
    lv_coord_t height = lv_obj_get_content_height(msgbox);
    lv_obj_set_height(content, height - title_height);
}

static lv_obj_t *add_help_text(lv_obj_t *list, const char *text) {
    lv_obj_t *label = lv_label_create(list);
    lv_label_set_text(label, text);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_set_style_text_font(label, lv_theme_get_font_small(list), 0);
    lv_obj_set_style_text_color(label, ml_color_hex(ML_COLOR_TEXT_MUTED), 0);
    return label;
}

static void add_section(lv_obj_t *list, const char *title) {
    lv_obj_t *label = add_help_text(list, title);
    lv_obj_set_style_text_font(label, lv_theme_get_font_normal(list), 0);
    lv_obj_set_style_text_color(label, ml_color_hex(ML_COLOR_PRIMARY), 0);
    lv_obj_set_style_pad_top(label, LV_DPX(16), 0);
}

static void add_shortcut(lv_obj_t *list, const char *keys, const char *action) {
    lv_obj_t *label = add_help_text(list, keys);
    lv_obj_set_style_text_color(label, ml_color_hex(ML_COLOR_TEXT), 0);
    lv_obj_set_style_pad_top(label, LV_DPX(8), 0);
    add_help_text(list, action);
}

static void add_about_line(lv_obj_t *list, const char *title, const char *value) {
    lv_obj_t *label = add_help_text(list, "");
    lv_label_set_text_fmt(label, "%s: %s", title, value != NULL ? value : locstr("not available"));
}

static const char *module_id(const SS4S_ModuleInfo *info) {
    const char *id = SS4S_ModuleInfoGetId(info);
    return id != NULL ? id : locstr("not available");
}
