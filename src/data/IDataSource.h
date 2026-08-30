#pragma once

#include <cstdint>

#include "model/BoatState.h"

namespace elixir {

class IDataSource {
public:
    virtual ~IDataSource() = default;
    virtual void begin(uint32_t now_ms) = 0;
    virtual void update(uint32_t now_ms, BoatState& state) = 0;
    virtual DataSourceKind kind() const = 0;
    virtual const char* label() const = 0;
};

}  // namespace elixir
