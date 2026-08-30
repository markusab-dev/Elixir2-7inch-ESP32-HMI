#pragma once

#include <memory>
#include <esp_display_panel.hpp>

namespace elixir::hal {
class DisplayRuntime {
public:
    bool begin();
    void setBacklight(bool enabled);
    bool backlightEnabled() const { return backlight_enabled_; }
private:
    std::unique_ptr<esp_panel::board::Board> board_{};
    bool backlight_enabled_{true};
};
}  // namespace elixir::hal
