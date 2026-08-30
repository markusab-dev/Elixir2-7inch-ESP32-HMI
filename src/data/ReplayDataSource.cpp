#include "data/ReplayDataSource.h"

#include <algorithm>
#include <cmath>
#include <ctime>

#include "AppConfig.h"

namespace elixir {
namespace {
float lerp(float a, float b, float t) { return a + (b - a) * t; }
double lerpDouble(double a, double b, float t) {
    return a + (b - a) * static_cast<double>(t);
}
float lerpAngle(float a, float b, float t) {
    const float delta = std::fmod(b - a + 540.0F, 360.0F) - 180.0F;
    float value = std::fmod(a + delta * t, 360.0F);
    return value < 0.0F ? value + 360.0F : value;
}
float centi(uint16_t value) { return static_cast<float>(value) / 100.0F; }
float deci(uint16_t value) { return static_cast<float>(value) / 10.0F; }
float signedDeci(int16_t value) { return static_cast<float>(value) / 10.0F; }
void setClock(BoatState& state, uint64_t unix_ms, int offset_minutes) {
    const std::time_t seconds = static_cast<std::time_t>(unix_ms / 1000ULL) +
                                static_cast<std::time_t>(offset_minutes * 60);
    std::tm tm_value{};
    gmtime_r(&seconds, &tm_value);
    state.clock_hour = static_cast<uint8_t>(tm_value.tm_hour);
    state.clock_minute = static_cast<uint8_t>(tm_value.tm_min);
    state.clock_second = static_cast<uint8_t>(tm_value.tm_sec);
}
}  // namespace

ReplayDataSource::ReplayDataSource(ReplayDescriptor descriptor)
    : descriptor_(descriptor) {}

void ReplayDataSource::begin(uint32_t now_ms) {
    elapsed_ms_ = 0;
    last_real_ms_ = now_ms;
    index_ = 0;
    playing_ = true;
}

void ReplayDataSource::update(uint32_t now_ms, BoatState& state) {
    if (descriptor_.records == nullptr || descriptor_.count < 2U) {
        state.source_kind = DataSourceKind::Replay;
        state.source_valid = false;
        return;
    }
    const uint32_t real_delta = now_ms - last_real_ms_;
    last_real_ms_ = now_ms;
    if (playing_) {
        const uint32_t advance = static_cast<uint32_t>(
            std::max(0.0F, static_cast<float>(real_delta) * speed_));
        const uint32_t duration = durationMs();
        if (duration > 0U) {
            const uint64_t next = static_cast<uint64_t>(elapsed_ms_) + advance;
            if (next >= duration) {
                if (config::kLoopReplay) {
                    elapsed_ms_ = static_cast<uint32_t>(next % duration);
                    index_ = 0;
                } else {
                    elapsed_ms_ = duration;
                    playing_ = false;
                }
            } else {
                elapsed_ms_ = static_cast<uint32_t>(next);
            }
        }
    }
    locateIndex();
    decodeInterpolated(state);
    state.source_kind = DataSourceKind::Replay;
    state.source_valid = true;
    state.source_elapsed_ms = elapsed_ms_;
    state.source_duration_ms = durationMs();
    state.source_unix_ms = descriptor_.start_unix_ms + elapsed_ms_;
    state.replay_playing = playing_;
    state.replay_speed = speed_;
    state.replay_progress = progress();
    setClock(state, state.source_unix_ms, descriptor_.timezone_offset_minutes);
}

void ReplayDataSource::togglePlaying() { playing_ = !playing_; }
void ReplayDataSource::setSpeed(float speed) {
    speed_ = std::clamp(speed, 0.1F, 100.0F);
}
void ReplayDataSource::seekNormalized(float progress_value) {
    const float p = std::clamp(progress_value, 0.0F, 1.0F);
    elapsed_ms_ = static_cast<uint32_t>(static_cast<float>(durationMs()) * p);
    index_ = 0;
    locateIndex();
}
float ReplayDataSource::progress() const {
    const uint32_t duration = durationMs();
    return duration == 0U ? 0.0F
                          : static_cast<float>(elapsed_ms_) / static_cast<float>(duration);
}
uint32_t ReplayDataSource::durationMs() const {
    return descriptor_.records == nullptr || descriptor_.count == 0U
               ? 0U
               : descriptor_.records[descriptor_.count - 1U].t_ms;
}
void ReplayDataSource::locateIndex() {
    if (descriptor_.count < 2U) {
        index_ = 0;
        return;
    }
    while (index_ + 1U < descriptor_.count - 1U &&
           descriptor_.records[index_ + 1U].t_ms <= elapsed_ms_) {
        ++index_;
    }
    while (index_ > 0U && descriptor_.records[index_].t_ms > elapsed_ms_) {
        --index_;
    }
}
void ReplayDataSource::decodeInterpolated(BoatState& state) const {
    const ReplayRecordPacked& a = descriptor_.records[index_];
    const ReplayRecordPacked& b =
        descriptor_.records[std::min(index_ + 1U, descriptor_.count - 1U)];
    const uint32_t span = b.t_ms > a.t_ms ? b.t_ms - a.t_ms : 1U;
    const float f = std::clamp(
        static_cast<float>(elapsed_ms_ - a.t_ms) / static_cast<float>(span),
        0.0F, 1.0F);
    state.latitude = lerpDouble(a.latitude_e7 / 1.0e7, b.latitude_e7 / 1.0e7, f);
    state.longitude = lerpDouble(a.longitude_e7 / 1.0e7, b.longitude_e7 / 1.0e7, f);
    state.sog_kn = lerp(centi(a.sog_centi_kn), centi(b.sog_centi_kn), f);
    state.stw_raw_kn = lerp(centi(a.stw_centi_kn), centi(b.stw_centi_kn), f);
    state.stw_kn = state.stw_raw_kn * state.stw_calibration_factor;
    state.cog_deg = lerpAngle(deci(a.cog_deci_deg), deci(b.cog_deci_deg), f);
    state.heading_deg = lerpAngle(deci(a.heading_deci_deg), deci(b.heading_deci_deg), f);
    state.aws_kn = lerp(centi(a.aws_centi_kn), centi(b.aws_centi_kn), f);
    state.awa_deg = lerp(signedDeci(a.awa_deci_deg), signedDeci(b.awa_deci_deg), f);
    state.depth_raw_m = lerp(deci(a.depth_deci_m), deci(b.depth_deci_m), f);
    state.depth_keel_m = std::max(0.0F, state.depth_raw_m + state.keel_offset_m);
    state.water_temp_c = lerp(signedDeci(a.water_temp_deci_c), signedDeci(b.water_temp_deci_c), f);
    state.roll_deg = lerp(signedDeci(a.roll_deci_deg), signedDeci(b.roll_deci_deg), f);
    state.pitch_deg = lerp(signedDeci(a.pitch_deci_deg), signedDeci(b.pitch_deci_deg), f);
}

}  // namespace elixir
