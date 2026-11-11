#!/bin/sh
set -eu

cd "$(dirname "$0")"

HOST="${FOODBRIDGE_HOST:-127.0.0.1}"
PORT="${FOODBRIDGE_PORT:-5000}"

if [ ! -f .env ]; then
  printf '%s\n' "FoodBridge is not configured yet." \
    "Follow the one-time setup in README.md and create the .env file first."
  exit 1
fi

if [ ! -x .venv/bin/flask ]; then
  printf '%s\n' "Preparing FoodBridge's local Python environment..."
  python3 -m venv .venv
  .venv/bin/python -m pip install -r requirements.txt
fi

if ! .venv/bin/flask --app app check-db; then
  printf '%s\n' "Check the MySQL settings in .env and make sure MySQL is running."
  exit 1
fi

if [ "$HOST" = "0.0.0.0" ]; then
  printf '%s\n' "FoodBridge is listening on port $PORT." \
    "On this computer, open http://127.0.0.1:$PORT." \
    "On a phone on the same Wi-Fi, open http://<computer-LAN-IP>:$PORT."
else
  printf '%s\n' "FoodBridge is starting at http://$HOST:$PORT"
fi

exec .venv/bin/python app.py
