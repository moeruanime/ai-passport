#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "lvgl.h"

static const char *TAG = "WEATHER_SIM";

// 模擬天氣數據
typedef struct {
    const char *city;
    const char *temp;
    const char *condition;
    const char *humidity;
} WeatherData;

static WeatherData city_list[] = {
    {"Taipei",   "26 C", "Partly Cloudy", "Humidity: 75%"},
    {"Tokyo",    "18 C", "Clear Sky",     "Humidity: 52%"},
    {"New York", "12 C", "Rainy",         "Humidity: 88%"},
    {"London",   "15 C", "Overcast",      "Humidity: 80%"}
};
static int current_city_index = 0;

static lv_obj_t *lbl_city;
static lv_obj_t *lbl_temp;
static lv_obj_t *lbl_desc;
static lv_obj_t *lbl_humidity;
static lv_obj_t *lbl_status;

// 更新 UI 顯示 (需持有官方 bsp_lvgl_lock)
static void update_weather_ui(int index) {
    if (bsp_lvgl_lock(pdMS_TO_TICKS(100))) {
        lv_label_set_text(lbl_city, city_list[index].city);
        lv_label_set_text(lbl_temp, city_list[index].temp);
        lv_label_set_text(lbl_desc, city_list[index].condition);
        lv_label_set_text(lbl_humidity, city_list[index].humidity);
        lv_label_set_text(lbl_status, "Press [OK] to Refresh");
        bsp_lvgl_unlock();
    }
}

// 建立 240x320 天氣介面
static void create_weather_ui(void) {
    if (!bsp_lvgl_lock(pdMS_TO_TICKS(500))) return;

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x10141E), 0);

    // 1. 頂部標題
    lv_obj_t *lbl_title = lv_label_create(scr);
    lv_label_set_text(lbl_title, "AI PASSPORT");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x8E99A4), 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 15);

    // 2. 城市名稱 (使用 20 號字型)
    lbl_city = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_city, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_city, lv_color_hex(0x00D2FF), 0);
    lv_obj_align(lbl_city, LV_ALIGN_TOP_MID, 0, 50);

    // 3. 溫度大字 (使用 20 號字型)
    lbl_temp = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_temp, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_temp, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_temp, LV_ALIGN_CENTER, 0, -20);

    // 4. 天氣描述 (使用 14 號字型)
    lbl_desc = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_desc, lv_color_hex(0xA8FF78), 0);
    lv_obj_align(lbl_desc, LV_ALIGN_CENTER, 0, 20);

    // 5. 濕度 (使用 14 號字型)
    lbl_humidity = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_humidity, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_humidity, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(lbl_humidity, LV_ALIGN_CENTER, 0, 50);

    // 6. 底部提示 (使用 14 號字型)
    lbl_status = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_status, lv_color_hex(0xFFA500), 0);
    lv_obj_align(lbl_status, LV_ALIGN_BOTTOM_MID, 0, -25);

    bsp_display_unlock();
}

// 按鍵監聽任務 (直接讀取 Passport 電阻分壓電壓)
static void button_task(void *arg) {
    bool pressed = false;
    while (1) {
        int mv = bsp_button_read_mv();
        
        // UP (~0mV), DOWN (~300mV), OK (~600mV), 放開 (>2000mV)
        if (!pressed) {
            if (mv < 150) { // UP 鍵
                current_city_index = (current_city_index - 1 + 4) % 4;
                update_weather_ui(current_city_index);
                pressed = true;
            } else if (mv >= 200 && mv <= 450) { // DOWN 鍵
                current_city_index = (current_city_index + 1) % 4;
                update_weather_ui(current_city_index);
                pressed = true;
            } else if (mv >= 500 && mv <= 850) { // OK 鍵
                if (bsp_lvgl_lock(pdMS_TO_TICKS(100))) {
                    lv_label_set_text(lbl_status, "Refreshing...");
                    bsp_lvgl_unlock();
                }
                vTaskDelay(pdMS_TO_TICKS(300));
                update_weather_ui(current_city_index);
                pressed = true;
            }
        } else {
            // 等待按鍵釋放
            if (mv > 2000) {
                pressed = false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting Passport Weather on Simulator...");

    // 1. 初始化官方螢幕與 LVGL
    bsp_display_init();
    bsp_lvgl_init();

    // 2. 建立 UI 畫面
    create_weather_ui();
    update_weather_ui(current_city_index);

    // 3. 啟動按鍵監聽任務
    xTaskCreate(button_task, "btn_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Application started!");
}
