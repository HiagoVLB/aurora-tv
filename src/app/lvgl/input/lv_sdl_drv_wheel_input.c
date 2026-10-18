#include "lvgl/lv_sdl_drv_input.h"

#include <SDL.h>

#include "app.h"
#include "stream/session.h"
#include "stream/session_events.h"
#include "ui/root.h"
#include "ui/ui_input.h"
#include "lv_gridview.h"

static void sdl_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data);

static bool wheel_handle_gridview(app_ui_input_t *input, const SDL_MouseWheelEvent *wheel) {
    if (wheel->y == 0) {
        return false;
    }
    lv_group_t *group = app_input_get_group(input);
    if (group == NULL) {
        return false;
    }
    lv_obj_t *focused = lv_group_get_focused(group);
    if (focused == NULL || !lv_obj_check_type(focused, &lv_gridview_class)) {
        return false;
    }
    /* Wheel up (y > 0 on webOS Magic Remote) pages toward earlier items. */
    lv_gridview_page(focused, wheel->y < 0);
    return true;
}

static bool wheel_is_scroll_container(lv_obj_t *obj) {
    /* Editable widgets can report a few pixels of internal text overflow. That
     * must not steal the wheel from the page containing the widget. */
    return lv_obj_is_visible(obj) && !lv_obj_is_group_def(obj) &&
           lv_obj_has_flag(obj, LV_OBJ_FLAG_SCROLLABLE) &&
           (lv_obj_get_scroll_dir(obj) & LV_DIR_VER) &&
           lv_obj_get_scroll_top(obj) + lv_obj_get_scroll_bottom(obj) > 0;
}

static void wheel_scroll(lv_obj_t *obj, int ticks) {
    const lv_coord_t step = LV_MAX(lv_dpx(48), lv_obj_get_height(obj) / 6);
    const int count = LV_MIN(SDL_abs(ticks), 8);
    lv_obj_scroll_by_bounded(obj, 0, (ticks > 0 ? step : -step) * count, LV_ANIM_OFF);
}

/* Keep the wheel in the active pane, including when its close button has focus. */
static bool wheel_handle_scrollable(app_ui_input_t *input, const SDL_MouseWheelEvent *wheel) {
    if (wheel->y == 0) {
        return false;
    }
    lv_group_t *group = app_input_get_group(input);
    if (group == NULL) {
        return false;
    }
    lv_obj_t *focused = lv_group_get_focused(group);
    if (focused == NULL) {
        return false;
    }
    if (lv_obj_has_class(focused, &lv_dropdown_class) && lv_dropdown_is_open(focused)) {
        wheel_scroll(lv_dropdown_get_list(focused), wheel->y);
        return true;
    }

    lv_obj_t *scrollable = focused;
    /* Do not search siblings on the screen/layer: those may belong to a dialog
     * behind the active one. A pane's header and body share a nearer parent. */
    while (scrollable != NULL && lv_obj_get_parent(scrollable) != NULL) {
        if (wheel_is_scroll_container(scrollable)) {
            wheel_scroll(scrollable, wheel->y);
            return true;
        }
        for (uint32_t i = 0; i < lv_obj_get_child_cnt(scrollable); i++) {
            lv_obj_t *child = lv_obj_get_child(scrollable, i);
            if (wheel_is_scroll_container(child)) {
                wheel_scroll(child, wheel->y);
                return true;
            }
        }
        scrollable = lv_obj_get_parent(scrollable);
    }
    return false;
}

int lv_sdl_init_wheel(lv_indev_drv_t *drv, app_ui_input_t *input) {
    lv_indev_drv_init(drv);
    drv->user_data = input;
    drv->type = LV_INDEV_TYPE_ENCODER;
    drv->read_cb = sdl_input_read;

    return 0;
}

static void sdl_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    app_ui_input_t *input = drv->user_data;
    SDL_Event e;
    data->state = LV_INDEV_STATE_RELEASED;
    data->enc_diff = 0;
    if (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_MOUSEWHEEL, SDL_MOUSEWHEEL)) {
        app_t *app = input->ui->app;
        if (app->session != NULL && !ui_should_block_input()) {
            session_handle_input_event(app->session, &e);
        } else if (wheel_handle_gridview(input, &e.wheel)) {
            /* Page the game grid; do not move group focus or scroll parents. */
        } else if (wheel_handle_scrollable(input, &e.wheel)) {
            /* Scroll settings / host lists instead of changing focus. */
        } else if (e.wheel.y != 0) {
            /* Encoder scroll only: never set PRESSED — that triggers LVGL "encoder button" clicks. */
            /* Match grid/page scrolling: SDL wheel down (y < 0) moves forward. */
            data->enc_diff = (e.wheel.y < 0) ? 1 : -1;
        }
    }
    data->continue_reading = false;
}
