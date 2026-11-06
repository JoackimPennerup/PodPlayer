#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
g++ -std=c++17 -Wall -Wextra -Werror -Wno-missing-field-initializers -pedantic \
  -Ifirmware/include firmware/src/RssFeedParser.cpp firmware/src/StateCodec.cpp \
  firmware/src/Ui.cpp firmware/test/test_main.cpp -o build/native-tests
./build/native-tests fixtures/sample-podcast.xml
node tests/core.test.cjs
