#pragma once

#include "data/IDataSource.h"

namespace elixir {

class SyntheticDataSource final : public IDataSource {
public:
    void begin(uint32_t now_ms) override;
    void update(uint32_t now_ms, BoatState& state) override;
    DataSourceKind kind() const override { return DataSourceKind::Synthetic; }
    const char* label() const override { return "Synthetic sea trial"; }

private:
    uint32_t start_ms_{0};
};

}  // namespace elixir
