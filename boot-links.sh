#!/bin/sh
# Recreate command symlinks after a reboot (or skip quietly if not installed).
set -u

DEST=${ZXTOOLS_HOME:-/usr/data/zxtools}
BIN=${ZXTOOLS_BIN:-/usr/bin}

[ -d "$DEST" ] || exit 0

if ! mkdir -p "$BIN" 2>/dev/null || [ ! -w "$BIN" ]; then
    BIN=$DEST/bin
    mkdir -p "$BIN" 2>/dev/null || exit 0
fi

for tool in zxtools router-check fileinfo zxnetmon; do
    if [ -x "$DEST/$tool" ]; then
        ln -sf "$DEST/$tool" "$BIN/$tool" || exit 0
    fi
done

exit 0
