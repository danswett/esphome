#!/usr/bin/env bash
set -euo pipefail

if ! command -v esphome >/dev/null 2>&1; then
  echo "esphome CLI is required for these tests" >&2
  exit 1
fi

SECRETS_FILE=${SECRETS_FILE:-tests/secrets.yaml}

if [ ! -f "$SECRETS_FILE" ]; then
  echo "Creating placeholder secrets file at $SECRETS_FILE" >&2
  cat <<'SECRETS' > "$SECRETS_FILE"
wifi_ssid: "TEST"
wifi_password: "PASSWORD"
SECRETS
fi

export ESPHOME_SECRETS="$SECRETS_FILE"

esphome config epaper-panel.yaml
esphome compile epaper-panel.yaml

g++ -std=c++17 -I. tests/refresh_logic_test.cpp -o tests/refresh_logic_test
./tests/refresh_logic_test

python3 -m unittest tests.test_refresh_script
