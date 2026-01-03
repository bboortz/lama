#!/bin/bash


set -e
set -u

CURFILE=$( readlink -f $0 )
CURDIR=${CURFILE%/*}
. ${CURDIR}/lib.sh


set -x

TARGET=x86_64-unknown-linux-musl

cargo test ${CARGO_PROFILE}
cargo build ${CARGO_PROFILE}

RUST_TARGET_FILE="target/${OUTPUT_PROFILE}/${TARGET_FILE}"
if [ ! -f "${RUST_TARGET_FILE}" ]; then
  echo "ERROR: target file missing: ${RUST_TARGET_FILE}"
  exit 1
fi
ls -la ${RUST_TARGET_FILE}
du -sh ${RUST_TARGET_FILE}
nm ${RUST_TARGET_FILE} | grep lamaPacketCallable || echo "ERROR: function lamaPacketCallable not exportet!"

./integration_tests/build_test.sh ${TARGET} ${OUTPUT_PROFILE}
echo "INTEGRATION TESTS OK!"

