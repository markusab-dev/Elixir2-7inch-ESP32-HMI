#pragma once

#include <cstdint>

namespace elixir {

enum class DataSourceKind : uint8_t { Synthetic = 0, Replay = 1, Live = 2 };

struct BoatState {
    DataSourceKind source_kind{DataSourceKind::Synthetic};
    uint64_t source_unix_ms{0};
    uint32_t source_elapsed_ms{0};
    uint32_t source_duration_ms{0};
    uint8_t clock_hour{20};
    uint8_t clock_minute{0};
    uint8_t clock_second{0};
    bool source_valid{false};
    bool replay_playing{true};
    float replay_speed{1.0F};
    float replay_progress{0.0F};

    // Navigation & Performance (Sun Fast 36 baseline)
    double latitude{59.3293};
    double longitude{18.0686};
    bool gps_valid{true};
    float sog_kn{0.0F};
    float stw_raw_kn{0.0F};
    float stw_calibration_factor{1.055F};  // STW reads ~5.5% low
    float stw_kn{0.0F};
    float cog_deg{0.0F};
    float heading_deg{0.0F};
    float magnetic_variation_deg{6.2F};

    // Wind (Garmin gWind delivers AWA/AWS; True Wind & VMG computed in MCU)
    float aws_kn{0.0F};
    float awa_deg{0.0F};
    float tws_kn{0.0F};
    float twa_deg{0.0F};
    float twd_deg{0.0F};
    float vmg_kn{0.0F};

    // Depth & Hull Attitude (DST triducer can0.35)
    float depth_raw_m{0.0F};              // belowTransducer
    float keel_offset_m{-1.40F};          // Transducer to keel bottom offset
    float depth_keel_m{0.0F};             // True depth below keel
    float water_temp_c{0.0F};
    float roll_deg{0.0F};                 // Heel (+ = Stbd, - = Port)
    float pitch_deg{0.0F};
    float trip_log_nm{24.8F};
    float total_log_nm{3840.0F};

    // Electrical (Victron Venus MQTT: SmartShunt & SmartSolar MPPT)
    float battery_soc_pct{0.0F};
    float battery_voltage_v{0.0F};
    float battery_current_a{0.0F};
    float battery_power_w{0.0F};
    float solar_power_w{0.0F};
    float solar_today_wh{0.0F};
    float estimated_hours_remaining{0.0F};

    // Machinery (Yanmar 3YM30 & ESP32-WROOM engineroom monitor .30)
    float engine_temp_c{0.0F};            // 85°C open thermostat, 93-97°C factory alarm
    float alternator_temp_c{0.0F};        // Custom limits: <80°C green, 80-100°C yellow, >100°C red
    float engine_room_temp_c{0.0F};
    bool engine_running{false};
    float generator_current_a{0.0F};      // Residual estimate: battery.current - solar.current

    // Comfort & Climate (Not yet installed - false until physical sensors exist)
    float cabin_temp_c{0.0F};
    float fridge_temp_c{0.0F};
    bool cabin_temp_valid{false};
    bool fridge_temp_valid{false};

    // Tactical & AIS (plotter can0.43)
    char ais_target_name[24]{"FINNFELLOW"};
    float ais_cpa_nm{1.42F};
    float ais_tcpa_min{18.5F};
    float ais_range_nm{3.85F};
    float ais_bearing_deg{142.0F};
    float ais_sog_kn{16.8F};
    uint16_t ais_targets_count{14};

    // Anchor Watch (signalk-anchoralarm plugin)
    bool anchor_active{false};
    float anchor_radius_m{12.4F};
    float anchor_max_radius_m{35.0F};
    float anchor_drift_bearing_deg{215.0F};
    bool anchor_alarm{false};

    // System Telemetry
    bool can_online{false};
    bool wifi_online{false};
    bool sd_online{false};
    bool victron_online{false};
    bool signalk_online{false};
    bool temperature_nodes_online{false};
    float n2k_frames_per_second{0.0F};
    uint16_t n2k_sources{0};
    uint32_t update_counter{0};
};

struct SystemSnapshot {
    uint32_t uptime_seconds{0};
    uint32_t flash_bytes{0};
    uint32_t sketch_bytes{0};
    uint32_t free_heap_bytes{0};
    uint32_t free_psram_bytes{0};
    uint32_t total_psram_bytes{0};
    bool psram_found{false};
    bool sd_mounted{false};
    uint64_t sd_total_bytes{0};
    uint64_t sd_used_bytes{0};
    float ui_refresh_hz{10.0F};
};

}  // namespace elixir
