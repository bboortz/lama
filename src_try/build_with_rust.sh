#!/bin/bash
# Build script for Arduino + Rust hybrid project
# This compiles the Arduino sketch and manually links the Rust library

set -e

PROJECT_DIR="/home/benni/git/lama"
SKETCH="lama.ino"
FQBN="esp32:esp32:esp32"
BUILD_DIR="$HOME/.cache/arduino/sketches/38677E5530EFBC237255C83E0CA6118E"

echo "=== Building Arduino sketch..."
arduino-cli compile --fqbn "$FQBN" "$SKETCH" 2>&1 | grep -v "undefined reference to.*lamaPacketCallable" || true

echo ""
echo "=== Manually linking with Rust library..."
cd "$BUILD_DIR"

# Re-run the final link command with the Rust library added at the END
/home/benni/.arduino15/packages/esp32/tools/esp-x32/2511/bin/xtensa-esp32-elf-g++ \
  -Wl,--Map="$BUILD_DIR/lama.ino.map" \
  -L/home/benni/.arduino15/packages/esp32/tools/esp32-arduino-libs/idf-release_v5.5-9bb7aa84-v2/esp32/lib \
  -L/home/benni/.arduino15/packages/esp32/tools/esp32-arduino-libs/idf-release_v5.5-9bb7aa84-v2/esp32/ld \
  -L/home/benni/.arduino15/packages/esp32/tools/esp32-arduino-libs/idf-release_v5.5-9bb7aa84-v2/esp32/qio_qspi \
  -Wl,--wrap=esp_panic_handler \
  @/home/benni/.arduino15/packages/esp32/tools/esp32-arduino-libs/idf-release_v5.5-9bb7aa84-v2/esp32/flags/ld_flags \
  @/home/benni/.arduino15/packages/esp32/tools/esp32-arduino-libs/idf-release_v5.5-9bb7aa84-v2/esp32/flags/ld_scripts \
  -Wl,--start-group \
  "$BUILD_DIR"/sketch/*.o \
  "$BUILD_DIR"/libraries/**/*.o \
  "$BUILD_DIR"/../cores/esp32_esp32_esp32_dbb3c2232e71ee56d967ac63433445c2/core.a \
  "$PROJECT_DIR/liblama_packet.a" \
  @/home/benni/.arduino15/packages/esp32/tools/esp32-arduino-libs/idf-release_v5.5-9bb7aa84-v2/esp32/flags/ld_libs \
  -Wl,--end-group \
  -Wl,-EL \
  -o "$BUILD_DIR/lama.ino.elf"

echo ""
echo "=== Build successful!"
ls -lh "$BUILD_DIR/lama.ino.elf"
