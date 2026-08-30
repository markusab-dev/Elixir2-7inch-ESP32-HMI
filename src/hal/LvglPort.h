#pragma once

#include <esp_display_panel.hpp>

namespace elixir::hal {
bool lvglPortInit(esp_panel::drivers::LCD* lcd,
                  esp_panel::drivers::Touch* touch);
bool lvglPortLock(int timeout_ms = -1);
void lvglPortUnlock();
bool lvglPortTouchPressed();
}  // namespace elixir::hal
