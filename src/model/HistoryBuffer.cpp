#include "model/HistoryBuffer.h"

#include <algorithm>

namespace elixir {

void HistoryBuffer::push(const BoatState& state) {
    HistorySample sample{};
    sample.sog = state.sog_kn;
    sample.stw = state.stw_kn;
    sample.aws = state.aws_kn;
    sample.tws = state.tws_kn;
    sample.voltage = state.battery_voltage_v;
    sample.current = state.battery_current_a;
    sample.solar = state.solar_power_w;
    sample.soc = state.battery_soc_pct;
    sample.engine_temp = state.engine_temp_c;
    sample.alternator_temp = state.alternator_temp_c;
    sample.fridge_temp = state.fridge_temp_c;
    sample.cabin_temp = state.cabin_temp_c;
    samples_[head_] = sample;
    head_ = (head_ + 1U) % kCapacity;
    size_ = std::min(size_ + 1U, kCapacity);
    ++sequence_;
}

const HistorySample& HistoryBuffer::oldest(std::size_t index) const {
    static const HistorySample empty{};
    if (index >= size_ || size_ == 0U) return empty;
    const std::size_t first = (head_ + kCapacity - size_) % kCapacity;
    return samples_[(first + index) % kCapacity];
}

const HistorySample& HistoryBuffer::newest() const {
    static const HistorySample empty{};
    if (size_ == 0U) return empty;
    return samples_[(head_ + kCapacity - 1U) % kCapacity];
}

}  // namespace elixir
