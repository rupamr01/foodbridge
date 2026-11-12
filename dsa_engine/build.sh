#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

g++ -std=c++14 -O2 -Wall -Wextra \
    -I "$ROOT/include" \
    "$ROOT"/src/*.cpp \
    -o "$ROOT/foodbridge_dsa"

echo "Built: $ROOT/foodbridge_dsa"
