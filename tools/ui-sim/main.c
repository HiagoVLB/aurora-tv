/* Host UI driver inspired by PlxNative's isolated simulator/input/capture loop.
 * Original implementation: runs Aurora's production SDL/LVGL UI, not webOS. */
#include "app.h"
#include "app_settings.h"
#include "ui/root.h"
#include "ui/launcher/launcher.controller.h"
#include "util/path.h"
#include <SDL_image.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static app_t app;
static const char *artifact_dir;

static int load_settings(app_settings_t *settings) {
    bool persistent;
    settings_initialize(settings, path_pref(&persistent));
    settings->conf_persistent = persistent;
    /* Fresh defaults every run; never read a developer's real host settings. */
    free(settings->decoder);
    settings->decoder = strdup("dummy");
    free(settings->audio_backend);
    settings->audio_backend = strdup("dummy");
    free(settings->language);
    const char *locale = getenv("AURORA_UI_SIM_LOCALE");
    settings->language = strdup(locale && locale[0] ? locale : "en");
    settings->fullscreen = false;
    settings->window_state = (window_state_t){.w = 1920, .h = 1080};
    const char *size = getenv("AURORA_UI_SIM_SIZE");
    if (size != NULL) {
        int width, height; char trailing;
        if (sscanf(size, "%dx%d%c", &width, &height, &trailing) != 2 ||
            width < 640 || height < 480 || width > 3840 || height > 2160) return -1;
        settings->window_state.w = width;
        settings->window_state.h = height;
    }
    settings->syskey_capture = false;
    settings->autoresume = false;
    return 0;
}

static void wait_ui(unsigned ms) {
    Uint32 start = SDL_GetTicks();
    while (app.running && SDL_GetTicks() - start < ms) app_run_loop(&app);
}

static void key_press(SDL_Keycode key) {
    SDL_Event event = {.key = {.type = SDL_KEYDOWN, .state = SDL_PRESSED,
        .keysym = {.sym = key, .scancode = SDL_GetScancodeFromKey(key)}}};
    SDL_PushEvent(&event);
    wait_ui(30);
    event.key.type = SDL_KEYUP;
    event.key.state = SDL_RELEASED;
    SDL_PushEvent(&event);
    wait_ui(250);
}

static void click_at(int x, int y) {
    SDL_Event event = {.motion = {.type = SDL_MOUSEMOTION, .x = x, .y = y}};
    SDL_PushEvent(&event);
    wait_ui(30);
    event.button = (SDL_MouseButtonEvent){.type = SDL_MOUSEBUTTONDOWN,
        .button = SDL_BUTTON_LEFT, .state = SDL_PRESSED, .x = x, .y = y};
    SDL_PushEvent(&event);
    wait_ui(30);
    event.button.type = SDL_MOUSEBUTTONUP;
    event.button.state = SDL_RELEASED;
    SDL_PushEvent(&event);
    wait_ui(250);
}

static lv_obj_t *find_text(lv_obj_t *root, const char *text, bool only_visible) {
    if (lv_obj_has_flag(root, LV_OBJ_FLAG_HIDDEN) || (only_visible && !lv_obj_is_visible(root))) return NULL;
    if (lv_obj_check_type(root, &lv_label_class) && strcmp(lv_label_get_text(root), text) == 0) return root;
    if (lv_obj_check_type(root, &lv_checkbox_class) && strcmp(lv_checkbox_get_text(root), text) == 0) return root;
    if (lv_obj_check_type(root, &lv_dropdown_class)) {
        char selected[256];
        lv_dropdown_get_selected_str(root, selected, sizeof(selected));
        const char *display = lv_dropdown_get_text(root);
        if (strcmp(display ? display : selected, text) == 0) return root;
    }
    if (lv_obj_check_type(root, &lv_btnmatrix_class)) {
        lv_btnmatrix_t *matrix = (lv_btnmatrix_t *) root;
        for (uint16_t i = 0; i < matrix->btn_cnt; i++) {
            if (strcmp(lv_btnmatrix_get_btn_text(root, i), text) == 0) return root;
        }
    }
    for (uint32_t i = 0; i < lv_obj_get_child_cnt(root); i++) {
        lv_obj_t *found = find_text(lv_obj_get_child(root, i), text, only_visible);
        if (found) return found;
    }
    return NULL;
}

static lv_obj_t *visible_text(const char *text) {
    lv_obj_t *found = find_text(lv_layer_top(), text, true);
    return found ? found : find_text(lv_scr_act(), text, true);
}

static bool output_path(char *path, size_t size, const char *name) {
    /* Artifacts are flat files within the run's private output directory. */
    if (!name[0] || name[0] == '.' || strchr(name, '/') || strchr(name, '\\')) return false;
    return snprintf(path, size, "%s/%s", artifact_dir, name) < (int) size;
}

static bool screenshot(const char *name) {
    char path[4096];
    if (!output_path(path, sizeof(path), name)) return false;
    lv_refr_now(app.ui.disp);
    SDL_Renderer *renderer = SDL_GetRenderer(app.ui.window);
    int w, h;
    SDL_Texture *target = SDL_GetRenderTarget(renderer);
    if (!target || SDL_QueryTexture(target, NULL, NULL, &w, &h) != 0) return false;
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) return false;
    bool ok = SDL_RenderReadPixels(renderer, NULL, surface->format->format, surface->pixels, surface->pitch) == 0;
    /* The TV draws the UI over a black/video plane. Flatten alpha for inspection. */
    if (ok) {
        SDL_Surface *flat = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
        if (flat) {
            SDL_FillRect(flat, NULL, SDL_MapRGBA(flat->format, 0, 0, 0, 255));
            SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
            SDL_BlitSurface(surface, NULL, flat, NULL);
            ok = IMG_SavePNG(flat, path) == 0;
            SDL_FreeSurface(flat);
        } else ok = false;
    }
    SDL_FreeSurface(surface);
    return ok;
}

static void json_string(FILE *file, const char *text) {
    fputc('"', file);
    for (const unsigned char *p = (const unsigned char *) text; *p; p++) {
        if (*p == '"' || *p == '\\') { fputc('\\', file); fputc(*p, file); }
        else if (*p < 32) fprintf(file, "\\u%04x", *p);
        else fputc(*p, file);
    }
    fputc('"', file);
}

static void dump_tree(FILE *file, lv_obj_t *obj, bool *first, int depth) {
    if (!lv_obj_is_visible(obj)) return;
    if (!*first) fputs(",\n", file);
    *first = false;
    lv_area_t area;
    lv_obj_get_coords(obj, &area);
    fprintf(file, "{\"depth\":%d,\"bounds\":[%d,%d,%d,%d],\"focused\":%s,\"disabled\":%s,\"clickable\":%s",
        depth, area.x1, area.y1, area.x2, area.y2,
        lv_obj_has_state(obj, LV_STATE_FOCUSED) ? "true" : "false",
        lv_obj_has_state(obj, LV_STATE_DISABLED) ? "true" : "false",
        lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE) ? "true" : "false");
    fprintf(file, ",\"scroll_y\":%d,\"scroll_bottom\":%d", lv_obj_get_scroll_y(obj), lv_obj_get_scroll_bottom(obj));
    const char *role = "object";
    if (lv_obj_check_type(obj, &lv_label_class)) role = "label";
    else if (lv_obj_check_type(obj, &lv_checkbox_class)) role = "checkbox";
    else if (lv_obj_check_type(obj, &lv_btn_class)) role = "button";
    else if (lv_obj_check_type(obj, &lv_btnmatrix_class)) role = "buttonmatrix";
    else if (lv_obj_check_type(obj, &lv_textarea_class)) role = "textbox";
    else if (lv_obj_check_type(obj, &lv_dropdown_class)) role = "combobox";
    else if (lv_obj_check_type(obj, &lv_slider_class)) role = "slider";
    fputs(",\"role\":", file);
    json_string(file, role);
    if (lv_obj_check_type(obj, &lv_dropdown_class)) {
        fprintf(file, ",\"selected\":%u,\"open\":%s", lv_dropdown_get_selected(obj),
                lv_dropdown_is_open(obj) ? "true" : "false");
    }
    if (lv_obj_check_type(obj, &lv_textarea_class)) {
        fputs(",\"value\":", file);
        json_string(file, lv_textarea_get_text(obj));
    }
    if (lv_obj_check_type(obj, &lv_label_class)) {
        fputs(",\"text\":", file);
        json_string(file, lv_label_get_text(obj));
    }
    if (lv_obj_check_type(obj, &lv_checkbox_class)) {
        fputs(",\"text\":", file);
        json_string(file, lv_checkbox_get_text(obj));
    }
    if (lv_obj_check_type(obj, &lv_btnmatrix_class)) {
        uint16_t selected = lv_btnmatrix_get_selected_btn(obj);
        fputs(",\"selected_button\":", file);
        const char *text = selected == LV_BTNMATRIX_BTN_NONE ? "" : lv_btnmatrix_get_btn_text(obj, selected);
        json_string(file, text ? text : "");
        fputs(",\"buttons\":[", file);
        lv_btnmatrix_t *matrix = (lv_btnmatrix_t *) obj;
        for (uint16_t i = 0; i < matrix->btn_cnt; i++) {
            if (i) fputc(',', file);
            json_string(file, lv_btnmatrix_get_btn_text(obj, i));
        }
        fputc(']', file);
    }
    fputc('}', file);
    for (uint32_t i = 0; i < lv_obj_get_child_cnt(obj); i++) dump_tree(file, lv_obj_get_child(obj, i), first, depth + 1);
}

static bool tree(const char *name) {
    char path[4096];
    if (!output_path(path, sizeof(path), name)) return false;
    FILE *file = fopen(path, "w");
    if (!file) return false;
    bool first = true;
    fputs("[\n", file);
    dump_tree(file, lv_scr_act(), &first, 0);
    dump_tree(file, lv_layer_top(), &first, 0);
    fputs("\n]\n", file);
    return fclose(file) == 0;
}

static bool command(char *line) {
    char *argument = strchr(line, ' ');
    if (argument) *argument++ = '\0';
    else argument = line + strlen(line);
    if (strcmp(line, "wait") == 0) {
        char *end;
        long ms = strtol(argument, &end, 10);
        if (end == argument || *end || ms < 0 || ms > 10000) return false;
        wait_ui((unsigned) ms);
        return true;
    }
    if (strcmp(line, "key") == 0) {
        SDL_Keycode key = SDL_GetKeyFromName(argument);
        if (!key) return false;
        key_press(key);
        return true;
    }
    if (strcmp(line, "wheel") == 0) {
        int ticks; char trailing;
        if (sscanf(argument, "%d %c", &ticks, &trailing) != 1 || ticks < -20 || ticks > 20) return false;
        SDL_Event event = {.wheel = {.type = SDL_MOUSEWHEEL, .y = ticks}};
        SDL_PushEvent(&event);
        wait_ui(150);
        return true;
    }
    if (strcmp(line, "resize") == 0) {
        int width, height; char trailing;
        if (sscanf(argument, "%d %d %c", &width, &height, &trailing) != 2 ||
            width < 640 || height < 480 || width > 3840 || height > 2160) return false;
        SDL_SetWindowSize(app.ui.window, width, height);
        wait_ui(300);
        return app.ui.width == width && app.ui.height == height;
    }
    if (strcmp(line, "text") == 0) {
        SDL_Event event = {.text = {.type = SDL_TEXTINPUT}};
        if (strlen(argument) >= sizeof(event.text.text)) return false;
        memcpy(event.text.text, argument, strlen(argument) + 1);
        SDL_PushEvent(&event);
        wait_ui(200);
        return true;
    }
    if (strcmp(line, "click") == 0) {
        int x, y; char trailing;
        if (sscanf(argument, "%d %d %c", &x, &y, &trailing) != 2 || x < 0 || y < 0 || x >= app.ui.width || y >= app.ui.height) return false;
        click_at(x, y);
        return true;
    }
    if (strcmp(line, "click_text") == 0 || strcmp(line, "scroll_text") == 0) {
        lv_obj_t *obj = find_text(lv_layer_top(), argument, false);
        if (!obj) obj = find_text(lv_scr_act(), argument, false);
        if (!obj) return false;
        lv_obj_scroll_to_view_recursive(obj, LV_ANIM_OFF);
        wait_ui(100);
        if (!lv_obj_is_visible(obj)) return false;
        if (strcmp(line, "scroll_text") == 0) return true;
        if (lv_obj_check_type(obj, &lv_btnmatrix_class)) {
            lv_btnmatrix_t *matrix = (lv_btnmatrix_t *) obj;
            lv_area_t bounds; lv_obj_get_coords(obj, &bounds);
            for (uint16_t i = 0; i < matrix->btn_cnt; i++) {
                if (strcmp(lv_btnmatrix_get_btn_text(obj, i), argument) != 0) continue;
                if (lv_btnmatrix_has_btn_ctrl(obj, i, LV_BTNMATRIX_CTRL_DISABLED)) return false;
                lv_area_t area = matrix->button_areas[i];
                click_at(bounds.x1 + (area.x1 + area.x2) / 2, bounds.y1 + (area.y1 + area.y2) / 2);
                return true;
            }
            return false;
        }
        while (obj && !lv_obj_has_flag(obj, LV_OBJ_FLAG_CLICKABLE)) obj = lv_obj_get_parent(obj);
        if (!obj || lv_obj_has_state(obj, LV_STATE_DISABLED)) return false;
        lv_area_t area; lv_obj_get_coords(obj, &area);
        click_at((area.x1 + area.x2) / 2, (area.y1 + area.y2) / 2);
        return true;
    }
    if (strcmp(line, "expect") == 0) return visible_text(argument) != NULL;
    if (strcmp(line, "absent") == 0) return visible_text(argument) == NULL;
    if (strcmp(line, "shot") == 0) return screenshot(argument);
    if (strcmp(line, "tree") == 0) return tree(argument);
    if (strcmp(line, "action") == 0) {
        launcher_fragment_t *launcher = launcher_instance();
        if (!launcher) return false;
        lv_obj_t *button = NULL;
        if (strcmp(argument, "settings") == 0) button = launcher->pref_btn;
        else if (strcmp(argument, "add_host") == 0) button = launcher->add_btn;
        else if (strcmp(argument, "help") == 0) button = launcher->help_btn;
        else if (strcmp(argument, "exit") == 0) button = launcher->quit_btn;
        if (!button || !lv_obj_is_visible(button)) return false;
        lv_area_t area; lv_obj_get_coords(button, &area);
        click_at((area.x1 + area.x2) / 2, (area.y1 + area.y2) / 2);
        return true;
    }
    if (strcmp(line, "quit") == 0) { app_request_exit(); return true; }
    return false;
}

int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "usage: aurora-ui-sim ARTIFACT_DIR\n"); return 2; }
    artifact_dir = argv[1];
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IOLBF, 0);
    if (app_init(&app, load_settings, argc, argv) != 0) return 1;
    app_ui_open(&app.ui, true, NULL);
    wait_ui(1000);
    puts("AURORA_SIM\tREADY");
    Uint32 started = SDL_GetTicks();
    while (app.running && SDL_GetTicks() - started < 300000) {
        app_run_loop(&app);
        struct pollfd input = {.fd = STDIN_FILENO, .events = POLLIN | POLLHUP};
        if (poll(&input, 1, 0) <= 0) continue;
        char line[4096];
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\r\n")] = 0;
        bool ok = command(line);
        printf("AURORA_SIM\t%s\n", ok ? "OK" : "ERROR");
    }
    app_deinit(&app);
    return 0;
}
