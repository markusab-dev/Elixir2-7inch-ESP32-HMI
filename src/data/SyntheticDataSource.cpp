#include "data/SyntheticDataSource.h"

#include <algorithm>
#include <cmath>
#include <ctime>

namespace elixir {
namespace {
constexpr uint64_t kStartUnixMs = 1786219200000ULL;
float wrap360(float value) {
    value = std::fmod(value, 360.0F);
    return value < 0.0F ? value + 360.0F : value;
}
void setClock(BoatState& state, uint64_t unix_ms) {
    const std::time_t seconds = static_cast<std::time_t>(unix_ms / 1000ULL) + 120 * 60;
    std::tm tm_value{};
    gmtime_r(&seconds, &tm_value);
    state.clock_hour = static_cast<uint8_t>(tm_value.tm_hour);
    state.clock_minute = static_cast<uint8_t>(tm_value.tm_min);
    state.clock_second = static_cast<uint8_t>(tm_value.tm_sec);
}
}  // namespace

void SyntheticDataSource::begin(uint32_t now_ms) { start_ms_ = now_ms; }
void SyntheticDataSource::update(uint32_t now_ms, BoatState& state) {
    const uint32_t elapsed_ms = now_ms - start_ms_;
    const float t = static_cast<float>(elapsed_ms) / 1000.0F;
    const float slow = std::sin(t * 0.035F);
    const float medium = std::sin(t * 0.11F + 0.9F);
    const float quick = std::sin(t * 0.39F + 1.6F);
    const float tack = std::sin(t * 0.011F) >= 0.0F ? 1.0F : -1.0F;

    state.source_kind = DataSourceKind::Synthetic;
    state.source_valid = true;
    state.source_elapsed_ms = elapsed_ms;
    state.source_duration_ms = 12U * 60U * 1000U;
    state.source_unix_ms = kStartUnixMs + elapsed_ms;
    state.replay_playing = true;
    state.replay_speed = 1.0F;
    state.replay_progress = static_cast<float>(elapsed_ms % state.source_duration_ms) /
                            static_cast<float>(state.source_duration_ms);
    state.sog_kn = 6.25F + 0.62F * slow + 0.18F * quick;
    state.stw_raw_kn = (state.sog_kn - 0.18F + 0.10F * medium) / state.stw_calibration_factor;
    state.stw_kn = state.stw_raw_kn * state.stw_calibration_factor;
    state.heading_deg = wrap360(137.0F + 10.0F * slow + 2.3F * quick);
    state.cog_deg = wrap360(state.heading_deg - 2.4F + 1.7F * medium);
    state.aws_kn = 14.8F + 2.8F * medium + 0.8F * quick;
    state.awa_deg = tack * (38.0F + 13.0F * slow - 4.0F * quick);
    state.depth_raw_m = 17.6F + 6.2F * std::sin(t * 0.018F + 0.5F);
    state.depth_keel_m = std::max(0.0F, state.depth_raw_m + state.keel_offset_m);
    state.water_temp_c = 17.4F + 0.18F * std::sin(t * 0.004F);
    state.roll_deg = -tack * (7.0F - 2.8F * medium - 1.1F * quick);
    state.pitch_deg = 0.9F + 1.2F * std::sin(t * 0.21F);
    state.latitude = 59.3020 + 0.00008 * static_cast<double>(t);
    state.longitude = 18.1780 + 0.00004 * static_cast<double>(t) +
                      0.0002 * static_cast<double>(std::sin(t * 0.01F));
    setClock(state, state.source_unix_ms);
}

}  // namespace elixir
