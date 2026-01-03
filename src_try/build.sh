#!/bin/bash
set -e
set -u

echo "=== Building Rust library ==="
cd lama-packet
make clean
rustup run esp cargo build --release --target xtensa-esp32-none-elf -Zbuild-std=core,alloc
cp target/xtensa-esp32-none-elf/release/liblama_packet.a ../
cd ..

echo "=== Building firmware ==="
pio run -t clean 
pio run
pio run --target upload --upload-port /dev/ttyACM1
pio device monitor --port /dev/ttyACM1

echo "=== Done! ==="
echo "To upload: pio run --target upload"

