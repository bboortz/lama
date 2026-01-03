#!/bin/bash

set -e
set -u 

CURFILE=$( readlink -f $0 )
CURDIR=${CURFILE%/*}
. ${CURDIR}/lib.sh

set -x

cd ../lama-core-rust
PROFILE=release make build
cd -

cd ../lama-packet-rust
PROFILE=release make build
cd -

pio run
pio run --target size
du -shc .pio/build/esp32dev/firmware.elf
