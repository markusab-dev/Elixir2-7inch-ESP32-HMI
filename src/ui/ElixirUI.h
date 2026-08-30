#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <lvgl.h>

#include "model/BoatState.h"
#include "model/HistoryBuffer.h"

namespace elixir::ui {

enum class Page : uint8_t {
    Overview = 0,
    Sail,
    Power,
    Engine,
    Tactical,
    System,
    Count
};

enum UiCommand : uint32_t {
    CommandNone = 0,
    CommandToggleSource = 1U << 0,
    CommandToggleReplay = 1U << 1,
    CommandCycleReplaySpeed = 1U << 2,
    CommandSeekReplay = 1U << 3,
    CommandToggleNightMode = 1U << 4,
    CommandScreenOff = 1U << 5,
    CommandScreenOn = 1U << 6,
    CommandCycleVisualBrightness = 1U << 7,
};

class ElixirUI {
public:
    void begin();
    void update(const BoatState& state, const HistoryBuffer& history,
                const SystemSnapshot& system, const char* source_label);
    uint32_t takeCommands();
    float pendingSeek() const { return pending_seek_.load(); }
    void setBacklightState(bool enabled);

private:
    static void navEvent(lv_event_t* event);
    static void commandEvent(lv_event_t* event);
    static void sliderEvent(lv_event_t* event);
    static void wakeEvent(lv_event_t* event);

    void buildChrome();
    void buildOverviewPage();
    void buildSailPage();
    void buildPowerPage();
    void buildEnginePage();
    void buildTacticalPage();
    void buildSystemPage();

    void showPage(Page page);
    void queueCommand(uint32_t command);
    void toggleNightOverlay();
    void cycleVisualBrightness();
    void refreshOverlayOrder();
    void appendChartSample(const HistorySample& sample);
    void updateCharts(const HistoryBuffer& history);

    lv_obj_t* createPage();
    lv_obj_t* createGlassCard(lv_obj_t* parent, int x, int y, int w, int h,
                              bool raised = false, int radius = 20);
    lv_obj_t* createCaption(lv_obj_t* parent, const char* text, int x, int y);
    lv_obj_t* createValue(lv_obj_t* parent, int x, int y, const lv_font_t* font,
                          lv_color_t color);
    lv_obj_t* createUnit(lv_obj_t* parent, const char* text, int x, int y);
    lv_obj_t* createActionButton(lv_obj_t* parent, const char* text, int x, int y,
                                 int w, int h, uint32_t command);
    lv_chart_series_t* addSeries(lv_obj_t* chart, lv_color_t color,
                                 lv_chart_axis_t axis = LV_CHART_AXIS_PRIMARY_Y);

    Page current_page_{Page::Overview};
    std::array<lv_obj_t*, static_cast<std::size_t>(Page::Count)> pages_{};
    std::array<lv_obj_t*, static_cast<std::size_t>(Page::Count)> nav_buttons_{};

    lv_obj_t* root_{nullptr};
    lv_obj_t* top_bar_{nullptr};
    lv_obj_t* floating_dock_{nullptr};
    lv_obj_t* source_label_{nullptr};
    lv_obj_t* clock_label_{nullptr};
    lv_obj_t* gnss_label_{nullptr};
    lv_obj_t* can_dot_{nullptr};
    lv_obj_t* wifi_dot_{nullptr};
    lv_obj_t* sd_dot_{nullptr};

    lv_obj_t* night_overlay_{nullptr};
    lv_obj_t* dim_overlay_{nullptr};
    lv_obj_t* wake_overlay_{nullptr};

    // Page 0: Overview (Master Dashboard)
    lv_obj_t* ov_sog_{nullptr};
    lv_obj_t* ov_stw_{nullptr};
    lv_obj_t* ov_depth_{nullptr};
    lv_obj_t* ov_heading_{nullptr};
    lv_obj_t* ov_wind_badge_{nullptr};
    lv_obj_t* ov_soc_arc_{nullptr};
    lv_obj_t* ov_soc_val_{nullptr};
    lv_obj_t* ov_soc_sub_{nullptr};
    lv_obj_t* ov_solar_val_{nullptr};
    lv_obj_t* ov_alt_charge_val_{nullptr};
    lv_obj_t* ov_engine_val_{nullptr};
    lv_obj_t* ov_alt_val_{nullptr};
    lv_obj_t* ov_climate_val_{nullptr};

    // Page 1: Sail (Helmsman & Performance HUD)
    lv_obj_t* sail_sog_{nullptr};
    lv_obj_t* sail_stw_{nullptr};
    lv_obj_t* sail_depth_{nullptr};
    lv_obj_t* sail_heading_{nullptr};
    lv_obj_t* sail_cog_{nullptr};
    lv_obj_t* sail_awa_val_{nullptr};
    lv_obj_t* sail_aws_val_{nullptr};
    lv_obj_t* sail_twa_val_{nullptr};
    lv_obj_t* sail_tws_val_{nullptr};
    lv_obj_t* sail_vmg_val_{nullptr};
    lv_obj_t* sail_roll_val_{nullptr};
    lv_obj_t* sail_pitch_val_{nullptr};
    lv_obj_t* sail_water_val_{nullptr};

    // Page 2: Power (Victron Energy, Solar & Alternator Thermal)
    lv_obj_t* power_soc_arc_{nullptr};
    lv_obj_t* power_soc_val_{nullptr};
    lv_obj_t* power_voltage_val_{nullptr};
    lv_obj_t* power_current_val_{nullptr};
    lv_obj_t* power_power_val_{nullptr};
    lv_obj_t* power_remaining_val_{nullptr};
    lv_obj_t* power_solar_val_{nullptr};
    lv_obj_t* power_solar_today_val_{nullptr};
    lv_obj_t* power_alt_temp_val_{nullptr};
    lv_obj_t* power_alt_status_val_{nullptr};
    lv_obj_t* power_chart_{nullptr};
    lv_chart_series_t* power_voltage_series_{nullptr};
    lv_chart_series_t* power_solar_series_{nullptr};

    // Page 3: Engine (Yanmar 3YM30 Propulsion Deck)
    lv_obj_t* eng_temp_val_{nullptr};
    lv_obj_t* eng_temp_bar_{nullptr};
    lv_obj_t* eng_alt_val_{nullptr};
    lv_obj_t* eng_alt_bar_{nullptr};
    lv_obj_t* eng_room_val_{nullptr};
    lv_obj_t* eng_charge_val_{nullptr};
    lv_obj_t* eng_state_badge_{nullptr};
    lv_obj_t* eng_sog_val_{nullptr};
    lv_obj_t* eng_depth_val_{nullptr};
    lv_obj_t* eng_heading_val_{nullptr};
    lv_obj_t* eng_chart_{nullptr};
    lv_chart_series_t* eng_temp_series_{nullptr};
    lv_chart_series_t* eng_alt_series_{nullptr};

    // Page 4: Tactical (AIS Targets & Anchor Watch)
    lv_obj_t* ais_name_val_{nullptr};
    lv_obj_t* ais_range_val_{nullptr};
    lv_obj_t* ais_cpa_val_{nullptr};
    lv_obj_t* ais_tcpa_val_{nullptr};
    lv_obj_t* ais_sog_val_{nullptr};
    lv_obj_t* ais_count_val_{nullptr};
    lv_obj_t* anchor_status_val_{nullptr};
    lv_obj_t* anchor_radius_val_{nullptr};
    lv_obj_t* anchor_max_val_{nullptr};
    lv_obj_t* anchor_bearing_val_{nullptr};

    // Page 5: System (Trip Log, Calibration & Bus Diagnostics)
    lv_obj_t* sys_trip_val_{nullptr};
    lv_obj_t* sys_total_val_{nullptr};
    lv_obj_t* sys_keel_offset_val_{nullptr};
    lv_obj_t* sys_stw_factor_val_{nullptr};
    lv_obj_t* mode_value_{nullptr};
    lv_obj_t* replay_label_{nullptr};
    lv_obj_t* replay_play_button_label_{nullptr};
    lv_obj_t* replay_speed_button_label_{nullptr};
    lv_obj_t* brightness_button_label_{nullptr};
    lv_obj_t* replay_slider_{nullptr};
    lv_obj_t* replay_progress_label_{nullptr};
    lv_obj_t* flash_value_{nullptr};
    lv_obj_t* psram_value_{nullptr};
    lv_obj_t* heap_value_{nullptr};
    lv_obj_t* uptime_value_{nullptr};
    lv_obj_t* firmware_value_{nullptr};

    std::atomic<uint32_t> pending_commands_{0};
    std::atomic<float> pending_seek_{0.0F};
    uint32_t last_history_sequence_{0};
    bool night_mode_{false};
    uint8_t visual_brightness_index_{0};
};

}  // namespace elixir::ui
