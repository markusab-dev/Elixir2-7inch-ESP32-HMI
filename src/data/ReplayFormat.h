#pragma once

#include <cstddef>
#include <cstdint>

namespace elixir {

#pragma pack(push, 1)
struct ReplayRecordPacked {
    uint32_t t_ms;
    int32_t latitude_e7;
    int32_t longitude_e7;
    uint16_t sog_centi_kn;
    uint16_t stw_centi_kn;
    uint16_t cog_deci_deg;
    uint16_t heading_deci_deg;
    uint16_t aws_centi_kn;
    int16_t awa_deci_deg;
    uint16_t depth_deci_m;
    int16_t water_temp_deci_c;
    int16_t roll_deci_deg;
    int16_t pitch_deci_deg;
};
#pragma pack(pop)

static_assert(sizeof(ReplayRecordPacked) == 32,
              "Replay layout changed; update converter and decoder together");

struct ReplayDescriptor {
    const ReplayRecordPacked* records;
    std::size_t count;
    uint64_t start_unix_ms;
    int16_t timezone_offset_minutes;
    const char* label;
};

}  // namespace elixir
