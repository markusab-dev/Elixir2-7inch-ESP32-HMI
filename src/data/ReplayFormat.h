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
    uint16_t battery_soc_centi_pct;   // e.g. 5740 for 57.4%
    uint16_t battery_voltage_centi_v; // e.g. 1313 for 13.13 V
    int16_t battery_current_centi_a;  // e.g. -500 for -5.00 A
    uint16_t solar_power_w;           // e.g. 5 for 5 W
    int16_t engine_temp_deci_c;       // e.g. 576 for 57.6 °C
    int16_t alternator_temp_deci_c;   // e.g. 593 for 59.3 °C
    int16_t engine_room_temp_deci_c;  // e.g. 476 for 47.6 °C
};
#pragma pack(pop)

static_assert(sizeof(ReplayRecordPacked) == 46,
              "Replay layout changed; update converter and decoder together");

struct ReplayDescriptor {
    const ReplayRecordPacked* records;
    std::size_t count;
    uint64_t start_unix_ms;
    int16_t timezone_offset_minutes;
    const char* label;
};

}  // namespace elixir
