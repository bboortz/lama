#!/bin/bash

set -e
set -u

CURFILE=$( readlink -f $0 )
CURDIR=${CURFILE%/*}
. ${CURDIR}/lib.sh

set -x 


find . -maxdepth 4 \( -name "*.h" -o -name "*.cpp" \) -print0 \
  | xargs -0 clang-format -i --verbose
