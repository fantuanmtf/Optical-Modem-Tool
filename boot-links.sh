#!/bin/sh
# 开机重建 /usr/bin 下的 zxtools 命令链接。
# 由 install.sh --boot 追加到现有 /usr/data/burntest/burntest.sh。
set -u

DEST=${ZXTOOLS_HOME:-/usr/data/zxtools}
BIN=${ZXTOOLS_BIN:-/usr/bin}

[ -d "$DEST" ] || exit 0
if [ ! -w "$BIN" ]; then
    BIN=$DEST/bin
    mkdir -p "$BIN" 2>/dev/null || exit 0
fi

for cmd in zxtools router-check fileinfo zxnetmon; do
    [ -x "$DEST/$cmd" ] && ln -sf "$DEST/$cmd" "$BIN/$cmd" 2>/dev/null
done
exit 0
