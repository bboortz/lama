#!/bin/bash

set -e
set -u

CURFILE=$( readlink -f $0 )
CURDIR=${CURFILE%/*}
. ${CURDIR}/lib.sh

#set -x 



TIDY_FLAGS="-I./include -I../lama-packet-rust -I./src"

echo "Formatting code in $PWD..."
find . -type f \( -name "*.ino" -o -name "*.h" -o -name "*.cpp" -o -name "*.c" \) \
    -not -path "*/OLD/*" \
    -not -path "*/test/*" \
    -not -path "*/.pio/*" \
    -exec clang-format -i --verbose {} +


echo "Done!"
