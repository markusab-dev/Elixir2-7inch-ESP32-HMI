#pragma once

#include <cstddef>
#include <cstdint>

#include "data/IDataSource.h"
#include "data/ReplayFormat.h"

namespace elixir {

class ReplayDataSource final : public IDataSource {
public:
    explicit ReplayDataSource(ReplayDescriptor descriptor);
    void begin(uint32_t now_ms) override;
    void update(uint32_t now_ms, BoatState& state) override;
    DataSourceKind kind() const override { return DataSourceKind::Replay; }
    const char* label() const override { return descriptor_.label; }

    void togglePlaying();
    void setSpeed(float speed);
    void seekNormalized(float progress);
    float progress() const;
    uint32_t durationMs() const;

private:
    void locateIndex();
    void decodeInterpolated(BoatState& state) const;

    ReplayDescriptor descriptor_{};
    bool playing_{true};
    float speed_{1.0F};
    uint32_t elapsed_ms_{0};
    uint32_t last_real_ms_{0};
    std::size_t index_{0};
};

}  // namespace elixir
