#!/bin/bash

set -e
set -u 

CURFILE=$( readlink -f $0 )
CURDIR=${CURFILE%/*}
. ${CURDIR}/lib.sh

set -x


# cargo build ${CARGO_ARGS}

export PATH=$PWD/xtensa-esp32-elf/bin:$PATH
# rustup run esp cargo build --release --target xtensa-esp32-none-elf --no-default-features --features esp32 -Zbuild-std=core,alloc
rustup run esp cargo build ${CARGO_ARGS}  --no-default-features --features esp32 -Zbuild-std=core,alloc
