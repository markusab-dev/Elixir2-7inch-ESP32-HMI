#pragma once

#include <cstddef>
#include <cstdint>

#include "data/ReplayDataSource.h"
#include "data/SyntheticDataSource.h"
#include "model/BoatState.h"

namespace elixir {

class DataEngine {
public:
    DataEngine();
    void begin(uint32_t now_ms);
    void update(uint32_t now_ms);
    const BoatState& state() const { return state_; }
    void setMode(DataSourceKind mode, uint32_t now_ms);
    void cycleMode(uint32_t now_ms);
    DataSourceKind mode() const { return mode_; }
    const char* activeLabel() const;
    void toggleReplayPlaying();
    void cycleReplaySpeed();
    void seekReplay(float progress);

private:
    void deriveWindAndPerformance();
    void applyDemoAuxiliaryData();

    BoatState state_{};
    SyntheticDataSource synthetic_{};
    ReplayDataSource replay_;
    IDataSource* active_{nullptr};
    DataSourceKind mode_{DataSourceKind::Synthetic};
    std::size_t replay_speed_index_{0};
};

}  // namespace elixir
