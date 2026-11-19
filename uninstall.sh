#!/bin/sh
# Optical-Modem-Tool 卸载脚本（纯 shell）
set -u

DEST=${ZXTOOLS_HOME:-/usr/data/zxtools}
BIN=${ZXTOOLS_BIN:-/usr/bin}
BOOT_HOOK=${ZXTOOLS_BOOT_HOOK:-/usr/data/burntest/burntest.sh}

if [ "$(id -u)" != "0" ]; then
    echo "请用 root 运行（Telnet/SSH root shell）" >&2
    exit 1
fi

for cmd in zxtools router-check fileinfo zxnetmon; do
    for dir in "$BIN" "$DEST/bin"; do
        if [ -L "$dir/$cmd" ]; then
            target=$(readlink "$dir/$cmd")
            case "$target" in
                "$DEST/$cmd"|"$DEST/bin/$cmd")
                    rm -f "$dir/$cmd"
                    ;;
            esac
        fi
    done
done

if [ -f "$BOOT_HOOK" ] && grep -q 'ZXTOOLS_BOOT' "$BOOT_HOOK" 2>/dev/null; then
    cp "$BOOT_HOOK" "$BOOT_HOOK.zxtools.uninstall.bak" || exit 1
    if sed -i '/# ZXTOOLS_BOOT/d; /zxtools\/boot-links.sh/d' "$BOOT_HOOK" 2>/dev/null; then
        :
    else
        awk '!/# ZXTOOLS_BOOT/ && !/zxtools\/boot-links.sh/' \
            "$BOOT_HOOK" > "$BOOT_HOOK.tmp" || exit 1
        mv "$BOOT_HOOK.tmp" "$BOOT_HOOK" || exit 1
    fi
    echo "已移除开机钩子中的 ZXTOOLS_BOOT 段"
fi

rm -rf "$DEST"
echo "已卸载: $DEST"
