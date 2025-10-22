#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>

#include "custom_components/refresh_logic/refresh_logic.h"

using esphome::refresh_logic::RefreshAction;
using esphome::refresh_logic::RefreshContext;
using esphome::refresh_logic::decide;

namespace {

void test_waits_for_missing_sensors() {
  RefreshContext ctx;
  ctx.sensors_ready = false;
  ctx.charging_state_available = true;
  ctx.charging_state = true;
  assert(decide(ctx) == RefreshAction::WAIT_FOR_DATA);
}

void test_stays_awake_when_charging() {
  RefreshContext ctx;
  ctx.sensors_ready = true;
  ctx.charging_state_available = true;
  ctx.charging_state = true;
  assert(decide(ctx) == RefreshAction::STAY_AWAKE);
}

void test_deep_sleep_when_not_charging() {
  RefreshContext ctx;
  ctx.sensors_ready = true;
  ctx.charging_state_available = true;
  ctx.charging_state = false;
  assert(decide(ctx) == RefreshAction::ENTER_DEEP_SLEEP);
}

void test_deep_sleep_when_charger_unknown() {
  RefreshContext ctx;
  ctx.sensors_ready = true;
  ctx.charging_state_available = false;
  ctx.charging_state = false;
  assert(decide(ctx) == RefreshAction::ENTER_DEEP_SLEEP);
}

struct SimulatedStep {
  bool sensors_ready;
  bool charger_known;
  bool charging;
  RefreshAction expected_action;
};

void test_simulated_refresh_sequence() {
  std::vector<SimulatedStep> sequence = {
      {false, false, false, RefreshAction::WAIT_FOR_DATA},
      {false, true, true, RefreshAction::WAIT_FOR_DATA},
      {true, true, true, RefreshAction::STAY_AWAKE},
      {true, true, true, RefreshAction::STAY_AWAKE},
      {true, true, false, RefreshAction::ENTER_DEEP_SLEEP},
      {true, false, false, RefreshAction::ENTER_DEEP_SLEEP},
  };

  for (std::size_t i = 0; i < sequence.size(); ++i) {
    RefreshContext ctx;
    ctx.sensors_ready = sequence[i].sensors_ready;
    ctx.charging_state_available = sequence[i].charger_known;
    ctx.charging_state = sequence[i].charging;
    auto action = decide(ctx);
    if (action != sequence[i].expected_action) {
      std::cerr << "Unexpected action at index " << i << std::endl;
    }
    assert(action == sequence[i].expected_action);
  }
}

}  // namespace

int main() {
  test_waits_for_missing_sensors();
  test_stays_awake_when_charging();
  test_deep_sleep_when_not_charging();
  test_deep_sleep_when_charger_unknown();
  test_simulated_refresh_sequence();
  std::cout << "All refresh logic tests passed." << std::endl;
  return 0;
}

