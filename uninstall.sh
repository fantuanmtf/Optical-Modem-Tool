#!/bin/sh
# Remove files installed by Optical-Modem-Tool/install.sh.
set -u

SRC=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
MODE=
DEST=
BIN=
KEEP_BOOT=0
BOOT_HOOK=${ZXTOOLS_BOOT_HOOK:-/usr/data/burntest/burntest.sh}

usage() {
    cat <<'EOF'
Usage: uninstall.sh [scenario] [options]

Scenarios:
  --modem          modem install: /usr/data/zxtools + /usr/bin links
  --local          local user install
  --prefix DIR     custom install directory
  --bin-dir DIR    custom symlink directory

Options:
  --keep-boot      keep the ZXTOOLS_BOOT block in the boot hook
  -h, --help       show this help
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --modem) MODE=modem ;;
        --local) MODE=local ;;
        --prefix)
            shift
            [ $# -gt 0 ] || { echo "uninstall: --prefix needs DIR" >&2; exit 2; }
            DEST=$1
            MODE=custom
            ;;
        --bin-dir)
            shift
            [ $# -gt 0 ] || { echo "uninstall: --bin-dir needs DIR" >&2; exit 2; }
            BIN=$1
            ;;
        --keep-boot) KEEP_BOOT=1 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "uninstall: unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
    shift
done

if [ -z "$MODE" ]; then
    if [ "$(id -u)" = 0 ] && [ -d /usr/data ]; then
        MODE=modem
    else
        MODE=local
    fi
fi

case "$MODE" in
    modem)
        DEST=${DEST:-${ZXTOOLS_HOME:-/usr/data/zxtools}}
        BIN=${BIN:-${ZXTOOLS_BIN:-/usr/bin}}
        ;;
    local)
        DEST=${DEST:-${ZXTOOLS_HOME:-${XDG_DATA_HOME:-$HOME/.local/share}/zxtools}}
        BIN=${BIN:-${ZXTOOLS_BIN:-$HOME/.local/bin}}
        ;;
    custom)
        [ -n "$DEST" ] || { echo "uninstall: --prefix is required for custom mode" >&2; exit 2; }
        BIN=${BIN:-${ZXTOOLS_BIN:-$DEST/bin}}
        ;;
esac

for tool in zxtools router-check fileinfo zxnetmon; do
    if [ -L "$BIN/$tool" ]; then
        target=$(readlink "$BIN/$tool" 2>/dev/null) || continue
        case "$target" in
            "$DEST/$tool"|"$DEST/bin/$tool") rm -f "$BIN/$tool" ;;
        esac
    fi
done

if [ "$KEEP_BOOT" = 0 ] && [ -f "$BOOT_HOOK" ] && grep -q 'ZXTOOLS_BOOT' "$BOOT_HOOK" 2>/dev/null; then
    cp "$BOOT_HOOK" "$BOOT_HOOK.zxtools.uninstall.bak" || exit 1
    sed -i -e '/# ZXTOOLS_BOOT/d' -e '/boot-links.sh/d' "$BOOT_HOOK" || exit 1
fi

if [ -n "$DEST" ] && [ -d "$DEST" ]; then
    rm -rf "$DEST" || exit 1
fi

echo "uninstalled: $DEST"
[ "$KEEP_BOOT" = 1 ] && echo "kept boot hook block"
exit 0
