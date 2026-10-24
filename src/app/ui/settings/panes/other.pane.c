#include "app.h"

#include "pref_obj.h"
#include "ui/settings/settings.controller.h"
#include "util/i18n.h"
#include "util/log_overlay.h"
#include "lvgl/util/lv_app_utils.h"
#include "app_settings.h"

#if TARGET_WEBOS
#include "platform/webos/game_mode.h"
#endif

#include <stdio.h>
#include <string.h>

typedef struct other_pane_t {
    lv_fragment_t base;
    settings_controller_t *parent;
    lv_obj_t *reset_mbox;
#if FEATURE_I18N_LANGUAGE_SETTINGS
    pref_dropdown_string_entry_t lang_entries[16];
    int lang_entries_len;
#endif
} other_pane_t;

static void pane_ctor(lv_fragment_t *self, void *args);
static void pane_dtor(lv_fragment_t *self);
static void reconnect_cb(lv_event_t *e);
static void reset_mbox_deleted(lv_event_t *e);

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container);

#if FEATURE_I18N_LANGUAGE_SETTINGS
static void language_changed_cb(lv_event_t *e);

#endif

static void reload_ui_after_locale(void *userdata);

static void on_show_logs_changed(lv_event_t *e);


static void reset_defaults_clicked(lv_event_t *e);

static void reset_defaults_confirm_cb(lv_event_t *e);

const lv_fragment_class_t settings_pane_other_cls = {
        .constructor_cb = pane_ctor,
        .destructor_cb = pane_dtor,
        .create_obj_cb = create_obj,
        .instance_size = sizeof(other_pane_t),
};

static void pane_ctor(lv_fragment_t *self, void *args) {
    other_pane_t *pane = (other_pane_t *) self;
    pane->parent = args;
#if FEATURE_I18N_LANGUAGE_SETTINGS
    pane->lang_entries_len = 0;
    for (int i = 0; i18n_entry_at(i)->locale && pane->lang_entries_len < 16; i++) {
        const i18n_entry_t *e = i18n_entry_at(i);
        pane->lang_entries[pane->lang_entries_len].name = locstr(e->name);
        pane->lang_entries[pane->lang_entries_len].value = e->locale;
        pane->lang_entries[pane->lang_entries_len].fallback = strcmp(e->locale, "auto") == 0;
        pane->lang_entries_len++;
    }
#endif
}

static lv_obj_t *create_obj(lv_fragment_t *self, lv_obj_t *container) {
    other_pane_t *pane = (other_pane_t *) self;
    lv_obj_t *view = pref_pane_container(container);
    lv_obj_set_layout(view, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(view, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(view, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    pref_header(view, locstr("Other"));

#if FEATURE_I18N_LANGUAGE_SETTINGS
    lv_obj_t *lang_row = pref_focus_row(view, locstr("Language"));
    lv_obj_t *lang_dd = pref_dropdown_string(lang_row, pane->lang_entries, (size_t) pane->lang_entries_len,
                                            &app_configuration->language);
    pref_style_inline_control(lang_dd);
    pref_row_bind_control(lang_row, lang_dd);
    lv_obj_add_event_cb(lang_dd, language_changed_cb, LV_EVENT_VALUE_CHANGED, pane);
#endif

    lv_obj_t *logs = pref_checkbox(view, locstr("Show logs"),
                                   &app_configuration->show_logs, false);
    lv_obj_add_event_cb(logs, on_show_logs_changed, LV_EVENT_VALUE_CHANGED, NULL);

#if TARGET_WEBOS
    if (webos_game_mode_is_rooted()) {
        lv_obj_t *game_mode = pref_checkbox(view, locstr("Game mode"), &app_configuration->game_mode, false);
        lv_obj_add_event_cb(game_mode, reconnect_cb, LV_EVENT_VALUE_CHANGED, pane);
    }
#endif

    pref_header(view, locstr("Reset"));
    lv_obj_t *reset_btn = lv_btn_create(view);
    pref_style_full_button(reset_btn);
    lv_obj_t *reset_lbl = lv_label_create(reset_btn);
    lv_label_set_text(reset_lbl, locstr("Reset all settings to defaults"));
    lv_obj_center(reset_lbl);
    lv_obj_add_event_cb(reset_btn, reset_defaults_clicked, LV_EVENT_CLICKED, pane);

    return view;
}

#if FEATURE_I18N_LANGUAGE_SETTINGS
static void language_changed_cb(lv_event_t *e) {
    other_pane_t *pane = lv_event_get_user_data(e);
    if (pane->parent) {
        pane->parent->needs_locale_reapply = true;
    }
    settings_save(app_configuration);
    if (app_configuration->language == NULL || app_configuration->language[0] == '\0' ||
        strcmp(app_configuration->language, "auto") == 0) {
        app_init_locale();
    } else {
        i18n_setlocale(app_configuration->language);
    }
    lv_async_call(reload_ui_after_locale, NULL);
}

#endif

static void reload_ui_after_locale(void *userdata) {
    (void) userdata;
    if (global == NULL) {
        return;
    }
    if (app_configuration->language == NULL || strcmp(app_configuration->language, "auto") == 0) {
        app_init_locale();
    } else {
        i18n_setlocale(app_configuration->language);
    }
    app_ui_reload(&global->ui);
}

static void reset_defaults_clicked(lv_event_t *e) {
    static const char *btns[] = {translatable("Cancel"), translatable("Reset"), ""};
    other_pane_t *pane = lv_event_get_user_data(e);
    if (pane->reset_mbox != NULL) return;
    lv_obj_t *mbox = lv_msgbox_create_i18n(NULL, locstr("Reset settings"),
                                           locstr("Restore all settings to defaults? Pairing is kept. "
                                                  "You must reconnect the stream."),
                                           btns, false);
    pane->reset_mbox = mbox;
    lv_obj_center(mbox);
    lv_obj_add_event_cb(mbox, reset_mbox_deleted, LV_EVENT_DELETE, pane);
    lv_obj_add_event_cb(mbox, reset_defaults_confirm_cb, LV_EVENT_VALUE_CHANGED,
                        lv_event_get_user_data(e));
}

static void reset_defaults_confirm_cb(lv_event_t *e) {
    other_pane_t *pane = lv_event_get_user_data(e);
    lv_obj_t *mbox = lv_event_get_current_target(e);
    if (lv_msgbox_get_active_btn(mbox) == 1 && app_configuration != NULL) {
        settings_restore_defaults(app_configuration);
        if (pane->parent) {
            pane->parent->needs_stream_reconnect = true;
        }
        log_overlay_set_enabled(app_configuration->show_logs);
        pane->parent->needs_locale_reapply = true;
    }
    bool reset = lv_msgbox_get_active_btn(mbox) == 1;
    lv_obj_remove_event_cb(mbox, reset_mbox_deleted);
    pane->reset_mbox = NULL;
    lv_msgbox_close_async(mbox);
    if (reset) lv_async_call(reload_ui_after_locale, NULL);
}

static void on_show_logs_changed(lv_event_t *e) {
    (void) e;
    log_overlay_set_enabled(app_configuration->show_logs);
}


static void pane_dtor(lv_fragment_t *self) {
    other_pane_t *pane = (other_pane_t *) self;
    if (pane->reset_mbox != NULL && lv_obj_is_valid(pane->reset_mbox)) {
        lv_obj_del(lv_obj_get_parent(pane->reset_mbox));
    }
}

static void reconnect_cb(lv_event_t *e) {
    other_pane_t *pane = lv_event_get_user_data(e);
    pane->parent->needs_stream_reconnect = true;
}

static void reset_mbox_deleted(lv_event_t *e) {
    other_pane_t *pane = lv_event_get_user_data(e);
    pane->reset_mbox = NULL;
}
