#include "hal/LvglPort.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <lvgl.h>

#include "AppConfig.h"

namespace elixir::hal {
namespace {
using esp_panel::drivers::LCD;
using esp_panel::drivers::Touch;
using esp_panel::drivers::TouchPoint;
SemaphoreHandle_t s_mutex = nullptr;
esp_timer_handle_t s_tick_timer = nullptr;
lv_color_t* s_buffer_one = nullptr;
lv_color_t* s_buffer_two = nullptr;
lv_disp_draw_buf_t s_draw_buffer{};
lv_disp_drv_t s_display_driver{};
lv_indev_drv_t s_touch_driver{};
std::atomic<bool> s_touch_pressed{false};

void flushCallback(lv_disp_drv_t* driver, const lv_area_t* area,
                   lv_color_t* color_map) {
    auto* lcd = static_cast<LCD*>(driver->user_data);
    const int width = area->x2 - area->x1 + 1;
    const int height = area->y2 - area->y1 + 1;
    if (lcd != nullptr) {
        lcd->drawBitmap(area->x1, area->y1, width, height,
                        reinterpret_cast<const uint8_t*>(color_map));
    }
    lv_disp_flush_ready(driver);
}

void touchReadCallback(lv_indev_drv_t* driver, lv_indev_data_t* data) {
    auto* touch = static_cast<Touch*>(driver->user_data);
    if (touch == nullptr) {
        s_touch_pressed.store(false, std::memory_order_relaxed);
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    TouchPoint points[1]{};
    const int count = touch->readPoints(points, 1, 0);
    if (count > 0 && points[0].x >= 0 && points[0].y >= 0) {
        data->point.x = static_cast<lv_coord_t>(points[0].x);
        data->point.y = static_cast<lv_coord_t>(points[0].y);
        data->state = LV_INDEV_STATE_PRESSED;
        s_touch_pressed.store(true, std::memory_order_relaxed);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        s_touch_pressed.store(false, std::memory_order_relaxed);
    }
}

void tickCallback(void*) { lv_tick_inc(2); }
void lvglTask(void*) {
    for (;;) {
        uint32_t wait_ms = 8;
        if (lvglPortLock(50)) {
            wait_ms = lv_timer_handler();
            lvglPortUnlock();
        }
        wait_ms = std::clamp<uint32_t>(wait_ms, 2U, 20U);
        vTaskDelay(pdMS_TO_TICKS(wait_ms));
    }
}

lv_color_t* allocateBuffer(std::size_t pixels) {
    auto* buffer = static_cast<lv_color_t*>(heap_caps_malloc(
        pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (buffer == nullptr) {
        buffer = static_cast<lv_color_t*>(heap_caps_malloc(
            pixels * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    }
    return buffer;
}
}  // namespace

bool lvglPortLock(int timeout_ms) {
    if (s_mutex == nullptr) return false;
    const TickType_t ticks = timeout_ms < 0 ? portMAX_DELAY
                                           : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(s_mutex, ticks) == pdTRUE;
}
void lvglPortUnlock() {
    if (s_mutex != nullptr) xSemaphoreGiveRecursive(s_mutex);
}

bool lvglPortTouchPressed() {
    return s_touch_pressed.load(std::memory_order_relaxed);
}

bool lvglPortInit(LCD* lcd, Touch* touch) {
    if (lcd == nullptr) return false;
    lv_init();
    s_mutex = xSemaphoreCreateRecursiveMutex();
    if (s_mutex == nullptr) return false;

    const std::size_t pixels = static_cast<std::size_t>(lcd->getFrameWidth()) *
                               config::kLvglBufferLines;
    s_buffer_one = allocateBuffer(pixels);
    if (s_buffer_one == nullptr) return false;
    if (config::kLvglBufferCount > 1U) s_buffer_two = allocateBuffer(pixels);
    lv_disp_draw_buf_init(&s_draw_buffer, s_buffer_one, s_buffer_two, pixels);

    lv_disp_drv_init(&s_display_driver);
    s_display_driver.hor_res = lcd->getFrameWidth();
    s_display_driver.ver_res = lcd->getFrameHeight();
    s_display_driver.flush_cb = flushCallback;
    s_display_driver.draw_buf = &s_draw_buffer;
    s_display_driver.user_data = lcd;
    lv_disp_drv_register(&s_display_driver);

    if (touch != nullptr) {
        lv_indev_drv_init(&s_touch_driver);
        s_touch_driver.type = LV_INDEV_TYPE_POINTER;
        s_touch_driver.read_cb = touchReadCallback;
        s_touch_driver.user_data = touch;
        lv_indev_drv_register(&s_touch_driver);
    }

    esp_timer_create_args_t timer_args{};
    timer_args.callback = &tickCallback;
    timer_args.dispatch_method = ESP_TIMER_TASK;
    timer_args.name = "lvgl_tick";
    timer_args.skip_unhandled_events = true;
    if (esp_timer_create(&timer_args, &s_tick_timer) != ESP_OK) return false;
    if (esp_timer_start_periodic(s_tick_timer, 2000) != ESP_OK) return false;
    return xTaskCreatePinnedToCore(lvglTask, "lvgl", 8192, nullptr, 3,
                                   nullptr, 1) == pdPASS;
}

}  // namespace elixir::hal
