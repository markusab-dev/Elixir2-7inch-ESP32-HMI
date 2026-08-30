#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "AppConfig.h"
#include "model/BoatState.h"

namespace elixir {

struct HistorySample {
    float sog{0.0F};
    float stw{0.0F};
    float aws{0.0F};
    float tws{0.0F};
    float voltage{0.0F};
    float current{0.0F};
    float solar{0.0F};
    float soc{0.0F};
    float engine_temp{0.0F};
    float alternator_temp{0.0F};
    float fridge_temp{0.0F};
    float cabin_temp{0.0F};
};

class HistoryBuffer {
public:
    static constexpr std::size_t kCapacity = config::kHistoryPoints;
    void push(const BoatState& state);
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    const HistorySample& oldest(std::size_t index) const;
    const HistorySample& newest() const;
    uint32_t sequence() const { return sequence_; }

private:
    std::array<HistorySample, kCapacity> samples_{};
    std::size_t head_{0};
    std::size_t size_{0};
    uint32_t sequence_{0};
};

}  // namespace elixir
