#include <Arduino.h>
#include <LVGL_CYD.h>
#include <ArduinoJson.h>
#include <RotaryEncoder.h>

#include "screens.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

// ─────────────────────────────────────────────
// CONFIG
// ─────────────────────────────────────────────
#define SERIAL_BAUD        115200
#define SCREEN_ORIENTATION USB_LEFT

#define IDLE_TIMEOUT_MS    (15UL * 60UL * 1000UL)   // 15 minutes
#define SERIAL_TIMEOUT_MS  10000UL                  // 10 seconds

// Rotary encoder pins
#define ENC_PIN_A    22
#define ENC_PIN_B    27
#define ENC_BTN_PIN  4

// Brightness levels
const uint8_t BRIGHT_NORMAL = 255;
const uint8_t BRIGHT_DIM    = 40;

// ─────────────────────────────────────────────
// STATE
// ─────────────────────────────────────────────

// ── SHARED STATE (Protected by mutexes) ──
SemaphoreHandle_t serialDataMutex = NULL;

// Flags indicating what needs to be updated
volatile bool updateTrackMetadata = false;
volatile bool updateTimeDisplay = false;

// Serial data (only modified by serial task)
StaticJsonDocument<1024> doc;
bool serialConnected       = false;
bool hasMedia              = false;
unsigned long lastPacket   = 0;
unsigned long lastMediaEvt = 0;
unsigned long noMediaStart = 0;

// UI state (can be modified by both serial and UI task)
struct PlaybackData {
    uint32_t position_ms = 0;
    uint32_t duration_ms = 0;
    int status = 0;
};

struct TrackMetadata {
    char title[256] = {0};
    char artist[256] = {0};
    char album[256] = {0};
    char app[64] = {0};
    uint16_t color565 = 0x001F;
    int mediaType = 0;
};

struct TimeData {
    char time[16] = {0};
    char date[16] = {0};
};

PlaybackData playback;
TrackMetadata track;
TimeData timeData;

// LVGL gradient state (RGB565)
uint16_t current_time_grad_565 = 0x001F;
uint16_t current_play_grad_565 = 0x001F;

// Lerp engine state
uint16_t start_play_grad_565 = 0;
uint16_t target_play_grad_565 = 0;
float play_lerp_progress = 1.0f;

// FreeRTOS tasks
TaskHandle_t encoderTaskHandle = NULL;
TaskHandle_t serialTaskHandle = NULL;

// ─────────────────────────────────────────────
// HELPERS
// ─────────────────────────────────────────────
void ms_to_mmss(uint32_t ms, char *out) {
    uint32_t totalSec = ms / 1000;
    uint32_t m = totalSec / 60;
    uint32_t s = totalSec % 60;
    sprintf(out, "%02u:%02u", (unsigned)m, (unsigned)s);
}

void updatePlayPauseIcon(int status) {
    if (!objects.play_pause_icon) return;
    const char *icon = (status == 4) ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY;
    lv_label_set_text(objects.play_pause_icon, icon);
}

void updateMediaTypeIcon(int mediaType) {
    if (!objects.media_type_icon__question_mark_if_unknown_) return;
    const char* icon;
    switch (mediaType) {
        case 1:  icon = LV_SYMBOL_AUDIO; break;
        case 2:  icon = LV_SYMBOL_VIDEO; break;
        default: icon = "?";             break;
    }
    lv_label_set_text(objects.media_type_icon__question_mark_if_unknown_, icon);
}

void showSerialDisconnected() {
    LVGL_CYD::backlight(BRIGHT_DIM);
    lv_scr_load(objects.serial_disconnect);
}

void showTimeScreenNormal() {
    LVGL_CYD::backlight(BRIGHT_NORMAL);
    lv_scr_load(objects.time_screen);
}

void showTimeScreenDimmed() {
    LVGL_CYD::backlight(BRIGHT_DIM);
    lv_scr_load(objects.time_screen_dimmed);
}

void showPlayScreen() {
    LVGL_CYD::backlight(BRIGHT_NORMAL);
    lv_scr_load(objects.play_screen);
}

// ─────────────────────────────────────────────
// COLOR HELPERS (RGB565 + gradients)
// ─────────────────────────────────────────────
static inline uint8_t r565(uint16_t c) { return ((c >> 11) & 0x1F) << 3; }
static inline uint8_t g565(uint16_t c) { return ((c >> 5)  & 0x3F) << 2; }
static inline uint8_t b565(uint16_t c) { return (c & 0x1F) << 3; }

static inline uint16_t rgb_to_565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) |
           ((g & 0xFC) << 3) |
           (b >> 3);
}

static inline lv_color_t color565_to_lv(uint16_t c) {
    return lv_color_make(r565(c), g565(c), b565(c));
}

uint16_t lerp565(uint16_t a, uint16_t b, uint8_t step) {
    uint8_t ar = r565(a);
    uint8_t ag = g565(a);
    uint8_t ab = b565(a);

    uint8_t br = r565(b);
    uint8_t bg = g565(b);
    uint8_t bb = b565(b);

    uint8_t rr = ar + ((int)br - ar) * step / 255;
    uint8_t rg = ag + ((int)bg - ag) * step / 255;
    uint8_t rb = ab + ((int)bb - ab) * step / 255;

    return rgb_to_565(rr, rg, rb);
}

// time-based target color (returns RGB565)
uint16_t computeTimeTargetColor565() {
    int hour = 12;
    if (timeData.time[0] && strlen(timeData.time) >= 2) {
        char buf[3] = { timeData.time[0], timeData.time[1], 0 };
        hour = atoi(buf);
        Serial.printf("DEBUG: time='%s' hour=%d\n", timeData.time, hour);
    }

    if (hour >= 5 && hour < 9) {      // sunrise
        return rgb_to_565(0xFF, 0xB3, 0x47);
    }
    if (hour >= 9 && hour < 13) {     // morning
        return rgb_to_565(0x4D, 0xA6, 0xFF);
    }
    if (hour >= 13 && hour < 18) {    // afternoon
        return rgb_to_565(0x66, 0xCC, 0xFF);
    }
    if (hour >= 18 && hour < 21) {    // sunset
        return rgb_to_565(0xCC, 0x66, 0xFF);
    }
    return rgb_to_565(0x00, 0x11, 0x33); // night
}

void updateTimeScreenGradient() {
    uint16_t target = computeTimeTargetColor565();
    current_time_grad_565 = lerp565(current_time_grad_565, target, 5);

    lv_color_t c = color565_to_lv(current_time_grad_565);

    if (objects.time_screen) {
        lv_obj_set_style_bg_grad_color(objects.time_screen, c, LV_PART_MAIN);
    }
    if (objects.time_screen_dimmed) {
        lv_obj_set_style_bg_grad_color(objects.time_screen_dimmed, c, LV_PART_MAIN);
    }
}

void updatePlayScreenGradient() {
    if (!hasMedia || !objects.play_screen) return;

    if (play_lerp_progress < 1.0f) {
        play_lerp_progress += 0.05f;
        if (play_lerp_progress > 1.0f) play_lerp_progress = 1.0f;

        current_play_grad_565 =
            lerp565(start_play_grad_565, target_play_grad_565,
                    (uint8_t)(play_lerp_progress * 255));
    }

    lv_obj_set_style_bg_color(objects.play_screen,
                              lv_color_make(28, 28, 28),
                              LV_PART_MAIN);

    lv_obj_set_style_bg_grad_color(objects.play_screen,
                                   color565_to_lv(current_play_grad_565),
                                   LV_PART_MAIN);

    lv_obj_set_style_bg_grad_dir(objects.play_screen,
                                 LV_GRAD_DIR_VER,
                                 LV_PART_MAIN);
}

// ─────────────────────────────────────────────
// UI UPDATE FUNCTIONS (only called from UI task)
// ─────────────────────────────────────────────
void updateTrackDisplay() {
    lv_label_set_text(objects.song_name,  track.title);
    lv_label_set_text(objects.artist,     track.artist);
    lv_label_set_text(objects.album_name, track.album);
    lv_label_set_text(objects.app_name,   track.app);

    updatePlayPauseIcon(playback.status);
    updateMediaTypeIcon(track.mediaType);

    // Reset lerp for new track
    start_play_grad_565 = current_play_grad_565;
    target_play_grad_565 = track.color565;
    play_lerp_progress = 0.0f;

    // Update progress bar
    if (objects.playtime_bar) {
        lv_bar_set_range(objects.playtime_bar, 0, playback.duration_ms ? playback.duration_ms : 1);
        lv_bar_set_value(objects.playtime_bar, playback.position_ms, LV_ANIM_OFF);
    }

    // Update time strings
    if (objects.playback_progress) {
        char buf[6];
        ms_to_mmss(playback.position_ms, buf);
        lv_label_set_text(objects.playback_progress, buf);
    }

    if (objects.total_playback_time) {
        char buf[6];
        ms_to_mmss(playback.duration_ms, buf);
        lv_label_set_text(objects.total_playback_time, buf);
    }
}

void updatePlaybackDisplay() {
    if (!hasMedia) return;

    // Update progress bar and time strings (these update frequently)
    if (objects.playtime_bar) {
        lv_bar_set_range(objects.playtime_bar, 0, playback.duration_ms ? playback.duration_ms : 1);
        lv_bar_set_value(objects.playtime_bar, playback.position_ms, LV_ANIM_OFF);
    }

    if (objects.playback_progress) {
        char buf[6];
        ms_to_mmss(playback.position_ms, buf);
        lv_label_set_text(objects.playback_progress, buf);
    }

    if (objects.total_playback_time) {
        char buf[6];
        ms_to_mmss(playback.duration_ms, buf);
        lv_label_set_text(objects.total_playback_time, buf);
    }

    updatePlayPauseIcon(playback.status);
}

void updateTimeDisplayLabels() {
    lv_label_set_text(objects.time,   timeData.time);
    lv_label_set_text(objects.date,   timeData.date);
    lv_label_set_text(objects.time_2, timeData.time);
    lv_label_set_text(objects.date_2, timeData.date);
}

// ─────────────────────────────────────────────
// SERIAL TASK (runs on core 0)
// ─────────────────────────────────────────────
void serialTask(void *param) {
    while (true) {
        if (Serial.available()) {
            String line = Serial.readStringUntil('\n');
            line.trim();

            if (line.length() == 0) {
                vTaskDelay(1);
                continue;
            }

            DeserializationError err = deserializeJson(doc, line);
            if (err) {
                vTaskDelay(1);
                continue;
            }

            lastPacket = millis();

            if (!serialConnected) {
                serialConnected = true;
                noMediaStart = millis();
            }

            const char* type = doc["type"] | "";

            if (strcmp(type, "track") == 0) {
                const char* title  = doc["title"]  | "";
                bool hasTitle = (strlen(title) > 0);

                if (!hasTitle) {
                    // No media
                    if (hasMedia) {
                        hasMedia = false;
                        noMediaStart = millis();
                    }
                } else {
                    // Media present
                    if (!hasMedia) {
                        hasMedia = true;
                        noMediaStart = 0;
                    }
                    lastMediaEvt = millis();

                    // Copy metadata (protected)
                    xSemaphoreTake(serialDataMutex, portMAX_DELAY);
                    {
                        strlcpy(track.title,   doc["title"]  | "", sizeof(track.title));
                        strlcpy(track.artist,  doc["artist"] | "", sizeof(track.artist));
                        strlcpy(track.album,   doc["album"]  | "", sizeof(track.album));
                        strlcpy(track.app,     doc["app"]    | "", sizeof(track.app));
                        track.color565 = doc["color"] | (uint16_t)0x001F;
                        track.mediaType = doc["media_type"] | 0;
                        updateTrackMetadata = true;
                    }
                    xSemaphoreGive(serialDataMutex);
                }

                // Always update playback position/duration (no flag needed)
                xSemaphoreTake(serialDataMutex, portMAX_DELAY);
                {
                    playback.position_ms = doc["position"] | (uint32_t)0;
                    playback.duration_ms = doc["duration"] | (uint32_t)0;
                    playback.status = doc["status"] | 0;
                }
                xSemaphoreGive(serialDataMutex);

                // Update time/date
                xSemaphoreTake(serialDataMutex, portMAX_DELAY);
                {
                    strlcpy(timeData.time, doc["time"] | "", sizeof(timeData.time));
                    strlcpy(timeData.date, doc["date"] | "", sizeof(timeData.date));
                    updateTimeDisplay = true;
                }
                xSemaphoreGive(serialDataMutex);

            } else if (strcmp(type, "state") == 0) {
                int status = doc["status"] | 0;

                xSemaphoreTake(serialDataMutex, portMAX_DELAY);
                {
                    playback.status = status;
                }
                xSemaphoreGive(serialDataMutex);

                if (hasMedia) {
                    lastMediaEvt = millis();
                }

                xSemaphoreTake(serialDataMutex, portMAX_DELAY);
                {
                    strlcpy(timeData.time, doc["time"] | "", sizeof(timeData.time));
                    strlcpy(timeData.date, doc["date"] | "", sizeof(timeData.date));
                    updateTimeDisplay = true;
                }
                xSemaphoreGive(serialDataMutex);

            } else if (strcmp(type, "clock") == 0) {
                xSemaphoreTake(serialDataMutex, portMAX_DELAY);
                {
                    strlcpy(timeData.time, doc["time"] | "", sizeof(timeData.time));
                    strlcpy(timeData.date, doc["date"] | "", sizeof(timeData.date));
                    updateTimeDisplay = true;
                }
                xSemaphoreGive(serialDataMutex);

            } else if (strcmp(type, "time") == 0) {
                if (hasMedia) {
                    xSemaphoreTake(serialDataMutex, portMAX_DELAY);
                    {
                        playback.position_ms = doc["position"] | (uint32_t)0;
                        playback.duration_ms = doc["duration"] | (uint32_t)0;
                    }
                    xSemaphoreGive(serialDataMutex);
                }
            }
        }

        vTaskDelay(1);
    }
}

// ─────────────────────────────────────────────
// ROTARY ENCODER
// ─────────────────────────────────────────────
RotaryEncoder rotaryEncoder(ENC_PIN_A, ENC_PIN_B, ENC_BTN_PIN);

void encoderTask(void *param) {
    while (true) {
        rotaryEncoder.update();
        int delta = rotaryEncoder.read();

        if (delta == -1) Serial.println("next");
        else if (delta == 1) Serial.println("prev");

        if (rotaryEncoder.click()) Serial.println("playpause");

        vTaskDelay(1);
    }
}

// ─────────────────────────────────────────────
// SETUP / LOOP
// ─────────────────────────────────────────────
void setup() {
    Serial.begin(SERIAL_BAUD);

    // Create mutex for serial data protection
    serialDataMutex = xSemaphoreCreateMutex();

    LVGL_CYD::begin(SCREEN_ORIENTATION);
    LVGL_CYD::backlight(BRIGHT_DIM);

    rotaryEncoder.enableLongPress(600);
    rotaryEncoder.setStepsPerDetent(4);
    rotaryEncoder.begin();

    // Serial task on core 0
    xTaskCreatePinnedToCore(
        serialTask,
        "SerialTask",
        2048,
        NULL,
        1,
        &serialTaskHandle,
        0
    );

    // Encoder task on core 0
    xTaskCreatePinnedToCore(
        encoderTask,
        "EncoderTask",
        2048,
        NULL,
        1,
        &encoderTaskHandle,
        0
    );

    create_screens();
    lv_scr_load(objects.serial_disconnect);
}

void loop() {
    // ── LVGL MAIN HANDLER (core 1) ──
    lv_task_handler();

    // ── UPDATE STATIC METADATA (triggered by flags) ──
    if (updateTrackMetadata) {
        xSemaphoreTake(serialDataMutex, portMAX_DELAY);
        updateTrackMetadata = false;
        xSemaphoreGive(serialDataMutex);

        if (hasMedia) {
            updateTrackDisplay();
            showPlayScreen();
        }
    }

    // ── UPDATE TIME DISPLAY (triggered by flag) ──
    if (updateTimeDisplay) {
        xSemaphoreTake(serialDataMutex, portMAX_DELAY);
        updateTimeDisplay = false;
        xSemaphoreGive(serialDataMutex);

        updateTimeDisplayLabels();
    }

    // ── CONTINUOUS UPDATES (every frame, no flags) ──
    // These update every loop iteration because playback position changes frequently
    if (hasMedia) {
        updatePlaybackDisplay();
    }

    // ── GRADIENT UPDATES (continuous) ──
    updateTimeScreenGradient();
    updatePlayScreenGradient();

    unsigned long now = millis();

    // ── SERIAL TIMEOUT ──
    if (serialConnected && (now - lastPacket > SERIAL_TIMEOUT_MS)) {
        serialConnected = false;
        hasMedia = false;
        showSerialDisconnected();
        lastPacket = 0;
        lastMediaEvt = 0;
    }

    // ── WAIT 10 SECONDS BEFORE SWITCHING TO TIME SCREEN ──
    if (!hasMedia && noMediaStart != 0) {
        if (now - noMediaStart >= 10000) {
            showTimeScreenNormal();
            noMediaStart = 0;
        }
    }

    // ── 15 MINUTES → DIMMED TIME SCREEN ──
    if (serialConnected && !hasMedia && lastMediaEvt != 0) {
        if (now - lastMediaEvt > IDLE_TIMEOUT_MS) {
            showTimeScreenDimmed();
        }
    }

    delay(1);
}

