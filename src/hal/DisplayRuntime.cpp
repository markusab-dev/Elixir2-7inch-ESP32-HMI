#include "hal/DisplayRuntime.h"

#include <Arduino.h>
#include "hal/LvglPort.h"

namespace elixir::hal {
bool DisplayRuntime::begin() {
    board_ = std::make_unique<esp_panel::board::Board>();
    if (!board_->init()) {
        Serial.println("[display] board init failed");
        return false;
    }
    if (!board_->begin()) {
        Serial.println("[display] board begin failed");
        return false;
    }
    if (!lvglPortInit(board_->getLCD(), board_->getTouch())) {
        Serial.println("[display] LVGL port init failed");
        return false;
    }
    setBacklight(true);
    return true;
}
void DisplayRuntime::setBacklight(bool enabled) {
    backlight_enabled_ = enabled;
    if (!board_ || board_->getBacklight() == nullptr) return;
    if (enabled) board_->getBacklight()->on();
    else board_->getBacklight()->off();
}
}  // namespace elixir::hal
