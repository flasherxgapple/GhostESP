#include "managers/views/calculator_screen.h"
#include "core/calculator.h"
#include "gui/design_tokens.h"
#include "gui/lvgl_safe.h"
#include "gui/theme_palette_api.h"
#include "lvgl.h"
#include "managers/settings_manager.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CALC_BUTTON_COUNT 20

static lv_obj_t *calc_root = NULL;
static lv_obj_t *calc_content = NULL;
static lv_obj_t *calc_display = NULL;
static char calc_input[128] = {0};
static bool calc_has_result = false;

static void calc_update_display(void) {
    if (!calc_display || !lv_obj_is_valid(calc_display)) return;
    lv_label_set_text(calc_display, calc_input[0] ? calc_input : "0");
}

static void calc_clear(void) {
    calc_input[0] = '\0';
    calc_has_result = false;
    calc_update_display();
}

static void calc_backspace(void) {
    size_t len = strlen(calc_input);
    if (len == 0) return;
    calc_input[len - 1] = '\0';
    calc_has_result = false;
    calc_update_display();
}

static void calc_append_text(const char *text) {
    if (!text || text[0] == '\0') return;
    if (calc_has_result) {
        if (isdigit((unsigned char)text[0]) || text[0] == '.') {
            calc_input[0] = '\0';
            calc_has_result = false;
        }
    }

    size_t current_len = strlen(calc_input);
    if (current_len + strlen(text) >= sizeof(calc_input)) {
        return;
    }
    strcat(calc_input, text);
    calc_update_display();
}

static void calc_evaluate(void) {
    if (calc_input[0] == '\0') return;

    double value = 0.0;
    char errbuf[128];
    if (!ghostesp_calculator_eval(calc_input, &value, errbuf, sizeof(errbuf))) {
        calc_input[0] = '\0';
        snprintf(calc_input, sizeof(calc_input), "%s", errbuf);
        calc_has_result = false;
        calc_update_display();
        return;
    }

    char formatted[64];
    ghostesp_calculator_format(value, formatted, sizeof(formatted));
    snprintf(calc_input, sizeof(calc_input), "%s", formatted);
    calc_has_result = true;
    calc_update_display();
}

static void calc_button_cb(lv_event_t *e) {
    const char *label = lv_event_get_user_data(e);
    if (!label) return;

    if (strcmp(label, "C") == 0) {
        calc_clear();
        return;
    }
    if (strcmp(label, "⌫") == 0) {
        calc_backspace();
        return;
    }
    if (strcmp(label, "=") == 0) {
        calc_evaluate();
        return;
    }

    calc_append_text(label);
}

static void calc_create(void) {
    int width = LV_HOR_RES;
    int height = LV_VER_RES;
    int status_h = GUI_STATUS_BAR_H;
    int content_h = height - status_h;
    if (content_h < 60) content_h = 60;

    uint8_t theme = settings_get_menu_theme(&G_Settings);
    lv_color_t bg = lv_color_hex(theme_palette_get_background(theme));
    lv_color_t surface = lv_color_hex(theme_palette_get_surface(theme));
    lv_color_t text = lv_color_hex(theme_palette_get_text(theme));
    lv_color_t accent = lv_color_hex(theme_palette_get_accent(theme));
    bool rounded = settings_get_menu_rounded(&G_Settings);
    lv_coord_t radius = rounded ? GUI_RADIUS_SM / 2 : 0;

    display_manager_fill_screen(bg);
    calc_root = gui_screen_create_root(NULL, NULL, bg, LV_OPA_TRANSP);
    calculator_view.root = calc_root;
    lv_obj_set_scrollbar_mode(calc_root, LV_SCROLLBAR_MODE_OFF);

    calc_content = lv_obj_create(calc_root);
    lv_obj_set_size(calc_content, width, content_h);
    lv_obj_align(calc_content, LV_ALIGN_TOP_MID, 0, status_h);
    lv_obj_set_style_bg_opa(calc_content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(calc_content, 0, 0);
    lv_obj_set_style_pad_all(calc_content, 0, 0);
    lv_obj_set_scrollbar_mode(calc_content, LV_SCROLLBAR_MODE_OFF);

    calc_display = lv_label_create(calc_content);
    lv_label_set_long_mode(calc_display, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_size(calc_display, width - 12, 46);
    lv_obj_align(calc_display, LV_ALIGN_TOP_MID, 0, 6);
    lv_obj_set_style_bg_color(calc_display, surface, 0);
    lv_obj_set_style_bg_opa(calc_display, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(calc_display, accent, 0);
    lv_obj_set_style_border_width(calc_display, 1, 0);
    lv_obj_set_style_radius(calc_display, radius, 0);
    lv_obj_set_style_text_color(calc_display, text, 0);
    lv_obj_set_style_text_font(calc_display, &lv_font_montserrat_20, 0);
    lv_obj_set_style_pad_all(calc_display, 10, 0);
    lv_label_set_text(calc_display, "0");

    lv_obj_t *grid = lv_obj_create(calc_content);
    lv_obj_set_size(grid, width - 12, content_h - 60);
    lv_obj_align(grid, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_pad_column(grid, 6, 0);
    lv_obj_set_style_pad_row(grid, 6, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    static const char *labels[CALC_BUTTON_COUNT] = {
        "7", "8", "9", "/", "C",
        "4", "5", "6", "*", "⌫",
        "1", "2", "3", "-", "(",
        "0", ".", "=", "+", ")"
    };

    for (int i = 0; i < CALC_BUTTON_COUNT; ++i) {
        lv_obj_t *btn = lv_btn_create(grid);
        lv_obj_set_size(btn, (width - 12 - 4 * 6) / 5, (content_h - 60 - 3 * 6) / 4);
        gui_apply_pressed_style(btn);
        lv_obj_set_style_bg_color(btn, surface, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_20, 0);
        lv_obj_set_style_border_color(btn, accent, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_radius(btn, radius, 0);
        lv_obj_add_event_cb(btn, calc_button_cb, LV_EVENT_CLICKED, (void *)labels[i]);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_center(lbl);
        lv_obj_set_style_text_color(lbl, text, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
    }

    display_manager_add_status_bar("Calculator");
    calc_update_display();
}

static void calc_destroy(void) {
    if (calc_root && lv_obj_is_valid(calc_root)) {
        lv_obj_clean(calc_root);
        lv_obj_del(calc_root);
    }
    calc_root = NULL;
    calc_content = NULL;
    calc_display = NULL;
    calc_input[0] = '\0';
    calc_has_result = false;
}

static void calc_input_handler(InputEvent *event) {
    if (!event) return;
    if (event->type == INPUT_TYPE_KEYBOARD) {
        int key = event->data.key_value;
        if (key >= '0' && key <= '9') {
            char tmp[2] = { (char)key, '\0' };
            calc_append_text(tmp);
            return;
        }
        if (key == '.' || key == ',') {
            calc_append_text(".");
            return;
        }
        if (key == '+' || key == '-' || key == '*' || key == '/' || key == '(' || key == ')') {
            char tmp[2] = { (char)key, '\0' };
            calc_append_text(tmp);
            return;
        }
        if (key == 8 || key == 127) {
            calc_backspace();
            return;
        }
        if (key == 13 || key == '\n' || key == '\r') {
            calc_evaluate();
            return;
        }
        if (key == 'c' || key == 'C') {
            calc_clear();
            return;
        }
    } else if (event->type == INPUT_TYPE_EXIT_BUTTON) {
        display_manager_go_back();
    } else if (event->type == INPUT_TYPE_ENCODER) {
        if (event->data.encoder.button) {
            calc_evaluate();
        } else if (event->data.encoder.direction > 0) {
            calc_append_text("+");
        } else if (event->data.encoder.direction < 0) {
            calc_append_text("-");
        }
    } else if (event->type == INPUT_TYPE_JOYSTICK) {
        if (event->data.joystick_pressed) {
            calc_evaluate();
        }
    }
}

static void calc_get_callback(void **callback) {
    if (!callback) return;
    *callback = calc_input_handler;
}

View calculator_view = {
    .root = NULL,
    .create = calc_create,
    .destroy = calc_destroy,
    .name = "Calculator",
    .get_hardwareinput_callback = calc_get_callback,
    .input_callback = calc_input_handler,
};
