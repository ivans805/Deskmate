#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *time_screen;
    lv_obj_t *time_screen_dimmed;
    lv_obj_t *serial_disconnect;
    lv_obj_t *play_screen;
    lv_obj_t *time;
    lv_obj_t *date;
    lv_obj_t *time_2;
    lv_obj_t *date_2;
    lv_obj_t *lv_usb_symbol;
    lv_obj_t *label_text;
    lv_obj_t *media_type_icon__question_mark_if_unknown_;
    lv_obj_t *app_name;
    lv_obj_t *metadata;
    lv_obj_t *play_pause_icon;
    lv_obj_t *artist;
    lv_obj_t *album_name;
    lv_obj_t *song_name;
    lv_obj_t *playtime_bar;
    lv_obj_t *playback_progress;
    lv_obj_t *total_playback_time;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_TIME_SCREEN = 1,
    SCREEN_ID_TIME_SCREEN_DIMMED = 2,
    SCREEN_ID_SERIAL_DISCONNECT = 3,
    SCREEN_ID_PLAY_SCREEN = 4,
};

void create_screen_time_screen();
void tick_screen_time_screen();

void create_screen_time_screen_dimmed();
void tick_screen_time_screen_dimmed();

void create_screen_serial_disconnect();
void tick_screen_serial_disconnect();

void create_screen_play_screen();
void tick_screen_play_screen();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/