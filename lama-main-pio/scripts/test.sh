#!/bin/bash


set -e
set -u

CURFILE=$( readlink -f $0 )
CURDIR=${CURFILE%/*}
. ${CURDIR}/lib.sh


#set -x

SUPPRESSIONS_FILE="cppcheck-suppressions.txt"

echo "Running cppcheck..."

# Create suppressions file if it doesn't exist
if [ ! -f "$SUPPRESSIONS_FILE" ]; then
    cat > "$SUPPRESSIONS_FILE" << 'EOF'
# Common suppressions for ESP32/Arduino projects
missingIncludeSystem
unmatchedSuppression
noExplicitConstructor
unusedFunction
EOF
    echo "Created $SUPPRESSIONS_FILE"
fi

# Run cppcheck
cppcheck \
    --enable=all \
    --std=c++11 \
    --platform=unix32 \
    --suppressions-list="$SUPPRESSIONS_FILE" \
    --language=c++ \
    --inline-suppr \
    --quiet \
    --template='{file}:{line}: {severity}: {message} [{id}]' \
    --file-list=<(find . -type f \( -name "*.ino" -o -name "*.cpp" -o -name "*.h" -o -name "*.c" \) \
        -not -path "*/OLD/*" \
        -not -path "*/test/*" \
        -not -path "*/.pio/*")

echo "cppcheck complete"
