#pragma once

#include <cstdint>

#include "data/DataEngine.h"
#include "hal/DisplayRuntime.h"
#include "model/HistoryBuffer.h"
#include "ui/ElixirUI.h"

namespace elixir {

class AppController {
public:
    bool begin();
    void loop();

private:
    void processUiCommands(uint32_t now_ms);
    void refreshSystemSnapshot(uint32_t now_ms);

    hal::DisplayRuntime display_{};
    DataEngine data_{};
    HistoryBuffer history_{};
    ui::ElixirUI ui_{};
    SystemSnapshot system_{};

    uint32_t last_data_ms_{0};
    uint32_t last_history_ms_{0};
    uint32_t last_ui_ms_{0};
    uint32_t last_system_ms_{0};
    bool wake_touch_armed_{false};
    bool ready_{false};
};

}  // namespace elixir
