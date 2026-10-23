#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "lvgl.h"

static const char *TAG = "WEATHER_SIM";

// 定義模擬天氣資料結構
typedef struct {
    const char *city;
    const char *temp;
    const char *condition;
    const char *humidity;
} WeatherData;

static WeatherData city_list[] = {
    {"Taipei",   "26°C", "Partly Cloudy", "Humidity: 75%"},
    {"Tokyo",    "18°C", "Clear Sky",     "Humidity: 52%"},
    {"New York", "12°C", "Rainy",         "Humidity: 88%"},
    {"London",   "15°C", "Overcast",      "Humidity: 80%"}
};
static int current_city_index = 0;

// LVGL UI 元件指針
static lv_obj_t *lbl_city;
static lv_obj_t *lbl_temp;
static lv_obj_t *lbl_desc;
static lv_obj_t *lbl_humidity;
static lv_obj_t *lbl_status;

// 更新 UI 畫面顯示
static void update_weather_ui(int index) {
    bsp_display_lock(0);
    lv_label_set_text(lbl_city, city_list[index].city);
    lv_label_set_text(lbl_temp, city_list[index].temp);
    lv_label_set_text(lbl_desc, city_list[index].condition);
    lv_label_set_text(lbl_humidity, city_list[index].humidity);
    lv_label_set_text(lbl_status, "Press [OK] to Refresh");
    bsp_display_unlock();
}

// 按鍵回調事件處理
static void button_event_cb(bsp_button_event_t event, void *user_data) {
    switch (event) {
        case BSP_BUTTON_UP_CLICK:
            current_city_index = (current_city_index - 1 + 4) % 4;
            ESP_LOGI(TAG, "UP Pressed -> City: %s", city_list[current_city_index].city);
            update_weather_ui(current_city_index);
            break;

        case BSP_BUTTON_DOWN_CLICK:
            current_city_index = (current_city_index + 1) % 4;
            ESP_LOGI(TAG, "DOWN Pressed -> City: %s", city_list[current_city_index].city);
            update_weather_ui(current_city_index);
            break;

        case BSP_BUTTON_OK_CLICK:
            ESP_LOGI(TAG, "OK Pressed -> Refreshing...");
            bsp_display_lock(0);
            lv_label_set_text(lbl_status, "Fetching Latest...");
            bsp_display_unlock();
            
            // 延遲 300ms 模擬網路請求完成
            vTaskDelay(pdMS_TO_TICKS(300));
            update_weather_ui(current_city_index);
            break;

        default:
            break;
    }
}

// 建立 240x320 天氣介面
static void create_weather_ui(void) {
    bsp_display_lock(0);

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x10141E), 0); // 深色背景

    // 1. 頂部標題列
    lv_obj_t *lbl_title = lv_label_create(scr);
    lv_label_set_text(lbl_title, "AI PASSPORT");
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(0x8E99A4), 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 15);

    // 2. 城市名稱
    lbl_city = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_city, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_city, lv_color_hex(0x00D2FF), 0);
    lv_obj_align(lbl_city, LV_ALIGN_TOP_MID, 0, 50);

    // 3. 溫度大字
    lbl_temp = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_temp, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(lbl_temp, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lbl_temp, LV_ALIGN_CENTER, 0, -20);

    // 4. 天氣描述
    lbl_desc = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_desc, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_desc, lv_color_hex(0xA8FF78), 0);
    lv_obj_align(lbl_desc, LV_ALIGN_CENTER, 0, 30);

    // 5. 濕度
    lbl_humidity = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_humidity, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_humidity, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(lbl_humidity, LV_ALIGN_CENTER, 0, 65);

    // 6. 底部按鍵提示與狀態
    lbl_status = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_status, lv_color_hex(0xFFA500), 0);
    lv_obj_align(lbl_status, LV_ALIGN_BOTTOM_MID, 0, -25);

    bsp_display_unlock();

    // 初始渲染
    update_weather_ui(current_city_index);
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting Weather Application on Simulator...");

    // 1. 初始化官方螢幕與 LVGL
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG()
    };
    bsp_display_init(&cfg);

    // 2. 初始化官方按鍵並註冊回調
    bsp_button_init();
    bsp_button_register_callback(BSP_BUTTON_UP_CLICK, button_event_cb, NULL);
    bsp_button_register_callback(BSP_BUTTON_DOWN_CLICK, button_event_cb, NULL);
    bsp_button_register_callback(BSP_BUTTON_OK_CLICK, button_event_cb, NULL);

    // 3. 建立天氣畫面
    create_weather_ui();

    ESP_LOGI(TAG, "Weather UI initialized successfully!");
}
