#!/bin/bash

set -e
set -u

CURFILE=$( readlink -f $0 )
CURDIR=${CURFILE%/*}
. ${CURDIR}/lib.sh

set -x 

TARGET=x86_64-unknown-linux-musl

cargo fmt
cargo clippy
