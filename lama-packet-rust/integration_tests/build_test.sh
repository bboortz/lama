#!/bin/bash

set -e
set -u
set -x 

CURFILE=$( readlink -f $0 )
CURDIR=${CURFILE%/*}

cd ${CURDIR}
echo "DIR: $(pwd)"

TARGET=${1:-x86_64-unknown-linux-musl}
OUTPUT_PROFILE=${2:-debug}
#TARGET_DIR="../target/${TARGET}/${OUTPUT_PROFILE}/"
TARGET_DIR="../target/${OUTPUT_PROFILE}/"
echo "TARGET_DIR: ${TARGET_DIR}"



echo
echo "*** TEST 01: c_integration_test ***"
rm -f c_integration_test
gcc -o c_integration_test c_integration_test.c -L ${TARGET_DIR} -llama_packet -lpthread -ldl
./c_integration_test
echo "TEST OK"
