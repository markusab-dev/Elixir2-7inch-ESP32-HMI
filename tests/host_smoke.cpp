#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "data/DataEngine.h"
#include "data/ReplayFormat.h"
#include "model/HistoryBuffer.h"

int main() {
    static_assert(sizeof(elixir::ReplayRecordPacked) == 46);

    elixir::DataEngine engine;
    engine.begin(1000U);
    assert(engine.mode() == elixir::DataSourceKind::Replay);
    const auto start = engine.state();
    assert(start.source_valid);
    assert(start.source_duration_ms > 0U);
    assert(start.replay_speed > 0.0F);

    for (uint32_t now = 1040U; now <= 12000U; now += 40U) {
        engine.update(now);
    }
    const auto replay = engine.state();
    assert(replay.sog_kn > 0.1F);
    assert(replay.aws_kn > 0.1F);
    assert(replay.tws_kn > 0.1F);
    assert(std::isfinite(replay.twa_deg));
    assert(replay.battery_voltage_v > 12.0F);
    assert(replay.engine_temp_c > 20.0F);

    const float before_seek = replay.replay_progress;
    engine.seekReplay(0.75F);
    engine.update(12040U);
    assert(engine.state().replay_progress > before_seek);
    assert(engine.state().replay_progress > 0.70F);

    engine.cycleMode(13000U);
    engine.update(13100U);
    assert(engine.mode() == elixir::DataSourceKind::Synthetic);
    assert(engine.state().source_kind == elixir::DataSourceKind::Synthetic);
    assert(engine.state().source_valid);

    elixir::HistoryBuffer history;
    for (int i = 0; i < 150; ++i) {
        engine.update(13200U + static_cast<uint32_t>(i) * 1000U);
        history.push(engine.state());
    }
    assert(history.size() == elixir::HistoryBuffer::kCapacity);
    assert(history.sequence() == 150U);
    assert(history.newest().sog > 0.1F);
    assert(history.oldest(0).aws > 0.1F);

    std::cout << "host_smoke: PASS\n";
    return 0;
}
