#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;
uint32_t active_theme_index = 0;

void create_screen_time_screen() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.time_screen = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff0096ff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // time
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.time = obj;
            lv_obj_set_pos(obj, 116, 20);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_inter_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "12:34");
        }
        {
            // date
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.date = obj;
            lv_obj_set_pos(obj, 108, 60);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff999999), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_inter_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "31.05.2026");
        }
    }
    
    tick_screen_time_screen();
}

void tick_screen_time_screen() {
}

void create_screen_time_screen_dimmed() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.time_screen_dimmed = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    {
        lv_obj_t *parent_obj = obj;
        {
            // time_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.time_2 = obj;
            lv_obj_set_pos(obj, 116, 20);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_inter_34, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "12:34");
        }
        {
            // date_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.date_2 = obj;
            lv_obj_set_pos(obj, 108, 60);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff999999), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_inter_20, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "31.05.2026");
        }
    }
    
    tick_screen_time_screen_dimmed();
}

void tick_screen_time_screen_dimmed() {
}

void create_screen_serial_disconnect() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.serial_disconnect = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    {
        lv_obj_t *parent_obj = obj;
        {
            // lv usb symbol
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.lv_usb_symbol = obj;
            lv_obj_set_pos(obj, 149, 95);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_label_set_text(obj, LV_SYMBOL_USB);
        }
        {
            // Label text
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_text = obj;
            lv_obj_set_pos(obj, 93, 111);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_inter_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Serial not connected");
        }
    }
    
    tick_screen_serial_disconnect();
}

void tick_screen_serial_disconnect() {
}

void create_screen_play_screen() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.play_screen = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 240);
    //lv_obj_set_style_bg_grad_dir(obj, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    //lv_obj_set_style_bg_grad_color(obj, lv_color_hex(0xff0046ff), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // media type icon (question mark if unknown)
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.media_type_icon__question_mark_if_unknown_ = obj;
            lv_obj_set_pos(obj, 288, 9);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, LV_SYMBOL_AUDIO);
        }
        {
            // app name
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.app_name = obj;
            lv_obj_set_pos(obj, 16, 8);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_inter_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Aplication");
        }
        {
            // metadata
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.metadata = obj;
            lv_obj_set_pos(obj, 7, 72);
            lv_obj_set_size(obj, 303, 97);
            lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff282b30), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 150, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // play pause icon
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.play_pause_icon = obj;
                    lv_obj_set_pos(obj, 255, 22);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_label_set_text(obj, LV_SYMBOL_PLAY);
                }
                {
                    // Artist
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.artist = obj;
                    lv_obj_set_pos(obj, -6, 34);
                    lv_obj_set_size(obj, 249, LV_SIZE_CONTENT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffbfbfbf), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_inter_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Artist(s)");
                }
                {
                    // Album name
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.album_name = obj;
                    lv_obj_set_pos(obj, -6, 52);
                    lv_obj_set_size(obj, 249, LV_SIZE_CONTENT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
                    lv_obj_set_style_text_font(obj, &ui_font_inter_12, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffcfcfcf), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Album");
                }
                {
                    // song name
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.song_name = obj;
                    lv_obj_set_pos(obj, -6, -8);
                    lv_obj_set_size(obj, 249, LV_SIZE_CONTENT);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
                    lv_obj_set_style_text_font(obj, &ui_font_inter_34, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Song");
                }
            }
        }
        {
            // playtime bar
            lv_obj_t *obj = lv_bar_create(parent_obj);
            objects.playtime_bar = obj;
            lv_obj_set_pos(obj, 7, 212);
            lv_obj_set_size(obj, 303, 18);
            lv_bar_set_value(obj, 25, LV_ANIM_OFF);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff888888), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 210, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // Playback progress
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.playback_progress = obj;
                    lv_obj_set_pos(obj, 4, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_inter_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "00:00");
                }
                {
                    // Total playback time
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.total_playback_time = obj;
                    lv_obj_set_pos(obj, 260, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_inter_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "00:00");
                }
            }
        }
    }
    
    tick_screen_play_screen();
}

void tick_screen_play_screen() {
}



typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_time_screen,
    tick_screen_time_screen_dimmed,
    tick_screen_serial_disconnect,
    tick_screen_play_screen,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_time_screen();
    create_screen_time_screen_dimmed();
    create_screen_serial_disconnect();
    create_screen_play_screen();
}
