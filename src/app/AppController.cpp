#include "app/AppController.h"

#include <Arduino.h>

#include "AppConfig.h"
#include "hal/LvglPort.h"

namespace elixir {

bool AppController::begin() {
    Serial.begin(config::kSerialBaud);
    delay(150);
    Serial.printf("\n[%s] firmware %s\n", config::kAppName, config::kVersion);
    Serial.printf("[%s] %s\n", config::kAppName, config::kBuildPurpose);

    if (!display_.begin()) {
        Serial.println("[app] display initialization failed");
        return false;
    }

    const uint32_t now_ms = millis();
    data_.begin(now_ms);
    history_.push(data_.state());
    refreshSystemSnapshot(now_ms);

    if (!hal::lvglPortLock(2000)) {
        Serial.println("[app] could not lock LVGL for UI creation");
        return false;
    }
    ui_.begin();
    ui_.update(data_.state(), history_, system_, data_.activeLabel());
    hal::lvglPortUnlock();

    last_data_ms_ = now_ms;
    last_history_ms_ = now_ms;
    last_ui_ms_ = now_ms;
    last_system_ms_ = now_ms;
    ready_ = true;
    Serial.println("[app] ready — touch the bottom navigation or open SYSTEM");
    return true;
}

void AppController::loop() {
    if (!ready_) {
        delay(500);
        return;
    }

    const uint32_t now_ms = millis();

    if (!display_.backlightEnabled()) {
        const bool pressed = hal::lvglPortTouchPressed();
        if (!wake_touch_armed_) {
            // Ignore the SCREEN OFF press until the finger has been released.
            wake_touch_armed_ = !pressed;
        } else if (pressed) {
            display_.setBacklight(true);
            if (hal::lvglPortLock(50)) {
                ui_.setBacklightState(true);
                hal::lvglPortUnlock();
            }
            wake_touch_armed_ = false;
        }
    }

    processUiCommands(now_ms);

    if (now_ms - last_data_ms_ >= config::kDataUpdateMs) {
        last_data_ms_ = now_ms;
        data_.update(now_ms);
    }
    if (now_ms - last_history_ms_ >= config::kHistorySampleMs) {
        last_history_ms_ = now_ms;
        history_.push(data_.state());
    }
    if (now_ms - last_system_ms_ >= config::kSystemRefreshMs) {
        last_system_ms_ = now_ms;
        refreshSystemSnapshot(now_ms);
    }
    if (now_ms - last_ui_ms_ >= config::kUiRefreshMs) {
        last_ui_ms_ = now_ms;
        if (hal::lvglPortLock(50)) {
            ui_.update(data_.state(), history_, system_, data_.activeLabel());
            hal::lvglPortUnlock();
        }
    }
    delay(2);
}

void AppController::processUiCommands(uint32_t now_ms) {
    const uint32_t commands = ui_.takeCommands();
    if (commands == ui::CommandNone) return;

    if ((commands & ui::CommandToggleSource) != 0U) {
        data_.cycleMode(now_ms);
        data_.update(now_ms);
    }
    if ((commands & ui::CommandToggleReplay) != 0U) {
        data_.toggleReplayPlaying();
    }
    if ((commands & ui::CommandCycleReplaySpeed) != 0U) {
        data_.cycleReplaySpeed();
    }
    if ((commands & ui::CommandSeekReplay) != 0U) {
        data_.seekReplay(ui_.pendingSeek());
        data_.update(now_ms);
    }
    if ((commands & ui::CommandScreenOff) != 0U) {
        if (hal::lvglPortLock(50)) {
            ui_.setBacklightState(false);
            hal::lvglPortUnlock();
        }
        delay(20);  // Ensure the black wake layer is rendered before BL_EN is cut.
        display_.setBacklight(false);
        wake_touch_armed_ = false;
    }
    if ((commands & ui::CommandScreenOn) != 0U) {
        display_.setBacklight(true);
        if (hal::lvglPortLock(50)) {
            ui_.setBacklightState(true);
            hal::lvglPortUnlock();
        }
    }
}

void AppController::refreshSystemSnapshot(uint32_t now_ms) {
    system_.uptime_seconds = now_ms / 1000U;
    system_.flash_bytes = ESP.getFlashChipSize();
    system_.sketch_bytes = ESP.getSketchSize();
    system_.free_heap_bytes = ESP.getFreeHeap();
    system_.psram_found = psramFound();
    system_.total_psram_bytes = ESP.getPsramSize();
    system_.free_psram_bytes = ESP.getFreePsram();
    // v0.1 deliberately does not mount SD; firmware and replay are self-contained.
    system_.sd_mounted = false;
    system_.sd_total_bytes = 0;
    system_.sd_used_bytes = 0;
    system_.ui_refresh_hz = 1000.0F / static_cast<float>(config::kUiRefreshMs);
}

}  // namespace elixir
