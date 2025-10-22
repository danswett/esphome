#pragma once

#include <cstdint>

namespace esphome {
namespace refresh_logic {

enum class RefreshAction : uint8_t {
  WAIT_FOR_DATA = 0,
  STAY_AWAKE = 1,
  ENTER_DEEP_SLEEP = 2,
};

struct RefreshContext {
  bool sensors_ready{false};
  bool charging_state_available{false};
  bool charging_state{false};
};

inline RefreshAction decide(const RefreshContext &ctx) {
  if (!ctx.sensors_ready) {
    return RefreshAction::WAIT_FOR_DATA;
  }
  if (ctx.charging_state_available && ctx.charging_state) {
    return RefreshAction::STAY_AWAKE;
  }
  return RefreshAction::ENTER_DEEP_SLEEP;
}

}  // namespace refresh_logic
}  // namespace esphome

