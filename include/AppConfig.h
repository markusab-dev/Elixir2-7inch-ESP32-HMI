#pragma once

#include <cstddef>
#include <cstdint>

namespace elixir::config {

inline constexpr char kAppName[] = "ELIXIR2 HMI";
inline constexpr char kVersion[] = "0.1.1-layout";
inline constexpr char kBuildPurpose[] = "GUI + synthetic/recorded data evaluation";

inline constexpr uint32_t kSerialBaud = 115200;
inline constexpr uint32_t kDataUpdateMs = 40;        // 25 Hz interpolation
inline constexpr uint32_t kUiRefreshMs = 100;       // 10 Hz numeric UI
inline constexpr uint32_t kHistorySampleMs = 1000;  // 1 Hz charts
inline constexpr uint32_t kSystemRefreshMs = 1000;

inline constexpr bool kStartInReplay = true;
inline constexpr bool kLoopReplay = true;
inline constexpr float kReplaySpeeds[] = {0.5F, 1.0F, 2.0F, 5.0F, 10.0F, 30.0F};
inline constexpr std::size_t kReplaySpeedCount =
    sizeof(kReplaySpeeds) / sizeof(kReplaySpeeds[0]);
inline constexpr std::size_t kInitialReplaySpeedIndex = 3;

inline constexpr uint16_t kHistoryPoints = 120;
inline constexpr uint16_t kLvglBufferLines = 48;
inline constexpr uint8_t kLvglBufferCount = 2;

}  // namespace elixir::config
