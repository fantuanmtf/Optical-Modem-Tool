#!/bin/sh
# Optical-Modem-Tool 安装脚本（纯 shell）
set -u

SRC=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
DEST=${ZXTOOLS_HOME:-/usr/data/zxtools}
BIN=${ZXTOOLS_BIN:-/usr/bin}
BOOT_HOOK=${ZXTOOLS_BOOT_HOOK:-/usr/data/burntest/burntest.sh}
WITH_BOOT=0

usage() {
    cat <<'EOF'
用法: install.sh [--boot]

  --boot   尝试把 boot-links.sh 追加到光猫开机钩子
           /usr/data/burntest/burntest.sh（默认不修改）

环境变量:
  ZXTOOLS_HOME       安装目录，默认 /usr/data/zxtools
  ZXTOOLS_BIN        命令链接目录，默认 /usr/bin
  ZXTOOLS_BOOT_HOOK  开机钩子路径
EOF
}

for arg in "$@"; do
    case "$arg" in
        --boot) WITH_BOOT=1 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "未知参数: $arg" >&2; usage >&2; exit 2 ;;
    esac
done

if [ "$(id -u)" != "0" ]; then
    echo "请用 root 运行（Telnet/SSH root shell）" >&2
    exit 1
fi

for f in zxtools/zxtools router-check/router-check fileinfo/fileinfo \
         zxnetmon/zxnetmon boot-links.sh; do
    if [ ! -f "$SRC/$f" ]; then
        echo "缺少文件: $SRC/$f" >&2
        exit 1
    fi
done

mkdir -p "$DEST" "$DEST/reports" || exit 1
cp "$SRC/zxtools/zxtools"       "$DEST/zxtools"       || exit 1
cp "$SRC/router-check/router-check" "$DEST/router-check" || exit 1
cp "$SRC/fileinfo/fileinfo"     "$DEST/fileinfo"      || exit 1
cp "$SRC/zxnetmon/zxnetmon"     "$DEST/zxnetmon"      || exit 1
cp "$SRC/boot-links.sh"         "$DEST/boot-links.sh" || exit 1
chmod 755 "$DEST/zxtools" "$DEST/router-check" "$DEST/fileinfo" \
          "$DEST/zxnetmon" "$DEST/boot-links.sh" || exit 1

if [ -w "$BIN" ]; then
    LINK_DIR=$BIN
else
    LINK_DIR=$DEST/bin
    mkdir -p "$LINK_DIR" || exit 1
fi

for cmd in zxtools router-check fileinfo zxnetmon; do
    ln -sf "$DEST/$cmd" "$LINK_DIR/$cmd" || {
        echo "创建链接失败: $LINK_DIR/$cmd" >&2
        exit 1
    }
done

if [ "$LINK_DIR" != "$BIN" ]; then
    echo "注意: $BIN 不可写，命令链接在 $LINK_DIR"
    echo "请把 $LINK_DIR 加入 PATH，或使用完整路径。"
fi

if [ "$WITH_BOOT" = "1" ]; then
    if [ -f "$BOOT_HOOK" ]; then
        if grep -q 'ZXTOOLS_BOOT' "$BOOT_HOOK" 2>/dev/null; then
            echo "开机钩子已包含 ZXTOOLS_BOOT，跳过"
        else
            cp "$BOOT_HOOK" "$BOOT_HOOK.zxtools.bak" || exit 1
            {
                printf '\n# ZXTOOLS_BOOT\n'
                printf '[ -x "%s/boot-links.sh" ] && "%s/boot-links.sh"\n' \
                    "$DEST" "$DEST"
            } >> "$BOOT_HOOK" || exit 1
            echo "已追加开机链接: $BOOT_HOOK"
        fi
    else
        echo "未找到 $BOOT_HOOK；跳过开机自启" >&2
        echo "可手动把 $DEST/boot-links.sh 加入启动脚本。" >&2
    fi
fi

echo "安装完成: $DEST"
echo "命令: zxtools / router-check / fileinfo / zxnetmon"
[ "$LINK_DIR" = "$BIN" ] || echo "PATH: export PATH=\"$LINK_DIR:\$PATH\""
