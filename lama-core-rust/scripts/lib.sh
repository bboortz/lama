
set -e
set -u

PROJECT_NAME="${PWD##*/}"
TARGET_FILE="liblama_core.a"
CODECOV_TOKEN=${CODECOV_TOKEN:-}

#TARGET=x86_64-unknown-linux-musl
TARGET=xtensa-esp32-none-elf
CARGO_PROFILE=""
CARGO_TARGET="--target ${TARGET}"
OUTPUT_PROFILE="debug"
if [ "$PROFILE" == "release" ]; then
  OUTPUT_PROFILE="release"
  CARGO_TARGET="--target ${TARGET}"
  CARGO_PROFILE="--release"
fi
CARGO_ARGS="${CARGO_PROFILE} ${CARGO_TARGET}"
RUST_TARGET_FILE="target/${TARGET}/${OUTPUT_PROFILE}/${TARGET_FILE}"

echo "**************************************"
echo "PROJECT_NAME: ${PROJECT_NAME}"
echo "OUTPUT_PROFILE: ${OUTPUT_PROFILE}"
echo "TARGET: ${TARGET}"
echo "CARGO: ${CARGO_ARGS}"
echo "RUST_TARGET_FILE: ${RUST_TARGET_FILE}"
echo "**************************************"
echo
