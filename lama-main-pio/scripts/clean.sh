#!/bin/bash

set -e
set -u 

CURFILE=$( readlink -f $0 )
CURDIR=${CURFILE%/*}
. ${CURDIR}/lib.sh


cd ../lama-packet-rust
PROFILE=release make clean
cd -

pio run --target clean
