#include "data/DataEngine.h"

#include <algorithm>
#include <cmath>

#include "AppConfig.h"
#include "generated/replay_data.h"

namespace elixir {
namespace {
constexpr float kPi = 3.14159265358979323846F;
constexpr float kDegToRad = kPi / 180.0F;
constexpr float kRadToDeg = 180.0F / kPi;
float clamp(float value, float minimum, float maximum) {
    return std::max(minimum, std::min(maximum, value));
}
float wrap360(float value) {
    value = std::fmod(value, 360.0F);
    return value < 0.0F ? value + 360.0F : value;
}
float wrapSigned180(float value) {
    value = std::fmod(value + 180.0F, 360.0F);
    if (value < 0.0F) value += 360.0F;
    return value - 180.0F;
}
}  // namespace

DataEngine::DataEngine() : replay_(generated::kReplayDescriptor) {}

void DataEngine::begin(uint32_t now_ms) {
    synthetic_.begin(now_ms);
    replay_.begin(now_ms);
    replay_speed_index_ = std::min(config::kInitialReplaySpeedIndex,
                                   config::kReplaySpeedCount - 1U);
    replay_.setSpeed(config::kReplaySpeeds[replay_speed_index_]);
    setMode(config::kStartInReplay ? DataSourceKind::Replay
                                   : DataSourceKind::Synthetic,
            now_ms);
    update(now_ms);
}

void DataEngine::update(uint32_t now_ms) {
    if (active_ == nullptr) return;
    active_->update(now_ms, state_);
    deriveWindAndPerformance();
    applyDemoAuxiliaryData();
    ++state_.update_counter;
}

void DataEngine::setMode(DataSourceKind mode, uint32_t now_ms) {
    if (mode == DataSourceKind::Live) mode = DataSourceKind::Replay;
    mode_ = mode;
    if (mode_ == DataSourceKind::Synthetic) {
        active_ = &synthetic_;
        synthetic_.begin(now_ms);
    } else {
        active_ = &replay_;
    }
}

void DataEngine::cycleMode(uint32_t now_ms) {
    setMode(mode_ == DataSourceKind::Replay ? DataSourceKind::Synthetic
                                            : DataSourceKind::Replay,
            now_ms);
}
const char* DataEngine::activeLabel() const {
    return active_ != nullptr ? active_->label() : "NO SOURCE";
}
void DataEngine::toggleReplayPlaying() {
    if (mode_ == DataSourceKind::Replay) replay_.togglePlaying();
}
void DataEngine::cycleReplaySpeed() {
    replay_speed_index_ = (replay_speed_index_ + 1U) % config::kReplaySpeedCount;
    replay_.setSpeed(config::kReplaySpeeds[replay_speed_index_]);
}
void DataEngine::seekReplay(float progress) {
    if (mode_ == DataSourceKind::Replay) replay_.seekNormalized(progress);
}

void DataEngine::deriveWindAndPerformance() {
    state_.stw_kn = state_.stw_raw_kn * state_.stw_calibration_factor;
    state_.depth_keel_m = std::max(0.0F, state_.depth_raw_m + state_.keel_offset_m);

    // True Wind computation in MCU (Garmin gWind delivers only AWA/AWS)
    const float awa_rad = state_.awa_deg * kDegToRad;
    const float apparent_flow_x = -state_.aws_kn * std::cos(awa_rad);
    const float apparent_flow_y = -state_.aws_kn * std::sin(awa_rad);
    const float true_flow_x = apparent_flow_x + state_.stw_kn;
    const float true_flow_y = apparent_flow_y;
    state_.tws_kn = std::sqrt(true_flow_x * true_flow_x + true_flow_y * true_flow_y);
    state_.twa_deg = wrapSigned180(std::atan2(-true_flow_y, -true_flow_x) * kRadToDeg);
    state_.twd_deg = wrap360(state_.heading_deg + state_.twa_deg);
    state_.vmg_kn = state_.stw_kn * std::cos(std::fabs(state_.twa_deg) * kDegToRad);
}

void DataEngine::applyDemoAuxiliaryData() {
    const float t = static_cast<float>(state_.source_elapsed_ms) / 1000.0F;
    const float warmup = 1.0F - std::exp(-t / 145.0F);
    const float solar_cycle = std::max(0.0F, std::sin((t / 720.0F) * kPi));
    const float load_wave = std::sin(t * 0.041F + 1.3F);

    // Yanmar 3YM30 & Engineroom
    state_.engine_running = (warmup > 0.35F);
    state_.engine_temp_c = 22.0F + 61.5F * warmup + 1.4F * std::sin(t * 0.025F);
    state_.alternator_temp_c = 24.0F + 48.0F * warmup + (state_.engine_running ? 14.0F : 0.0F) +
                               2.0F * std::sin(t * 0.032F);
    state_.engine_room_temp_c = 20.0F + 13.0F * warmup + 0.8F * std::sin(t * 0.015F);

    // Victron Electrical & Solar
    state_.solar_power_w = 420.0F * solar_cycle + 15.0F * std::sin(t * 0.12F);
    state_.solar_today_wh = 286.0F + (t / 3600.0F) * state_.solar_power_w * 0.8F;
    state_.generator_current_a = state_.engine_running ? (34.5F + 3.0F * std::sin(t * 0.04F)) : 0.0F;
    state_.battery_current_a = (state_.solar_power_w / 13.2F) + state_.generator_current_a - 6.5F + 0.35F * load_wave;
    state_.battery_voltage_v = 13.20F + state_.battery_current_a * 0.007F;
    state_.battery_power_w = state_.battery_voltage_v * state_.battery_current_a;
    state_.battery_soc_pct = clamp(92.0F + 0.6F * solar_cycle - t / 8000.0F, 0.0F, 100.0F);

    const float discharge_a = std::max(0.2F, -state_.battery_current_a);
    state_.estimated_hours_remaining = clamp(
        (state_.battery_soc_pct / 100.0F * 180.0F) / discharge_a, 0.0F, 99.0F);

    // Trip & Tactical
    state_.trip_log_nm = 24.8F + (t / 3600.0F) * state_.sog_kn;
    state_.total_log_nm = 3840.0F + state_.trip_log_nm;
    state_.gps_valid = true;

    // Anchor Watch
    state_.anchor_radius_m = 12.4F + 1.2F * std::sin(t * 0.02F);
    state_.anchor_max_radius_m = 35.0F;
    state_.anchor_drift_bearing_deg = wrap360(215.0F + 12.0F * std::sin(t * 0.01F));

    state_.can_online = false;
    state_.wifi_online = false;
    state_.sd_online = false;
    state_.victron_online = false;
    state_.temperature_nodes_online = false;
    state_.signalk_online = false;
    state_.n2k_frames_per_second = 282.0F + 24.0F * std::sin(t * 0.13F);
    state_.n2k_sources = 14;
}

}  // namespace elixir
