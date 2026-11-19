#!/bin/sh
# Optical-Modem-Tool installer: modem, local user, or custom prefix.
set -u

SRC=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
MODE=
DEST=
BIN=
WITH_BOOT=0
NO_LINKS=0
BOOT_HOOK=${ZXTOOLS_BOOT_HOOK:-/usr/data/burntest/burntest.sh}

usage() {
    cat <<'EOF'
Usage: install.sh [scenario] [options]

Scenarios:
  --modem          root on the modem: /usr/data/zxtools + /usr/bin links
  --local          normal user: ~/.local/share/zxtools + ~/.local/bin
  --prefix DIR     custom install directory
  --bin-dir DIR    custom symlink directory

Options:
  --boot           append a boot hook (modem mode only)
  --no-links       do not create command symlinks
  -h, --help       show this help

Environment:
  ZXTOOLS_HOME     override install directory
  ZXTOOLS_BIN      override symlink directory
  ZXTOOLS_BOOT_HOOK override boot hook path

With no scenario, auto-detects modem when root and /usr/data exists,
otherwise local user install.
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --modem) MODE=modem ;;
        --local) MODE=local ;;
        --prefix)
            shift
            [ $# -gt 0 ] || { echo "install: --prefix needs DIR" >&2; exit 2; }
            DEST=$1
            MODE=custom
            ;;
        --bin-dir)
            shift
            [ $# -gt 0 ] || { echo "install: --bin-dir needs DIR" >&2; exit 2; }
            BIN=$1
            ;;
        --boot) WITH_BOOT=1 ;;
        --no-links) NO_LINKS=1 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "install: unknown option: $1" >&2; usage >&2; exit 2 ;;
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
        [ "$(id -u)" = 0 ] || { echo "install: --modem requires root" >&2; exit 1; }
        DEST=${DEST:-${ZXTOOLS_HOME:-/usr/data/zxtools}}
        BIN=${BIN:-${ZXTOOLS_BIN:-/usr/bin}}
        ;;
    local)
        DEST=${DEST:-${ZXTOOLS_HOME:-${XDG_DATA_HOME:-$HOME/.local/share}/zxtools}}
        BIN=${BIN:-${ZXTOOLS_BIN:-$HOME/.local/bin}}
        ;;
    custom)
        [ -n "$DEST" ] || { echo "install: --prefix is required for custom mode" >&2; exit 2; }
        BIN=${BIN:-${ZXTOOLS_BIN:-$DEST/bin}}
        ;;
esac

if [ "$WITH_BOOT" = 1 ] && [ "$MODE" != modem ]; then
    echo "install: --boot is only supported with --modem" >&2
    exit 2
fi

mkdir -p "$DEST" "$DEST/reports" || exit 1

for tool in zxtools router-check fileinfo zxnetmon; do
    src=$SRC/src/$tool/$tool
    if [ ! -f "$src" ]; then
        echo "install: missing source: $src" >&2
        exit 1
    fi
    cp "$src" "$DEST/$tool" || exit 1
    chmod 755 "$DEST/$tool"
done

cp "$SRC/boot-links.sh" "$DEST/boot-links.sh" || exit 1
chmod 755 "$DEST/boot-links.sh"

LINK_DIR=$BIN
if [ "$NO_LINKS" = 0 ]; then
    if ! mkdir -p "$LINK_DIR" 2>/dev/null || [ ! -w "$LINK_DIR" ]; then
        LINK_DIR=$DEST/bin
        mkdir -p "$LINK_DIR" || exit 1
    fi
    for tool in zxtools router-check fileinfo zxnetmon; do
        ln -sf "$DEST/$tool" "$LINK_DIR/$tool" || exit 1
    done
fi

if [ "$WITH_BOOT" = 1 ]; then
    if [ ! -f "$BOOT_HOOK" ]; then
        echo "install: boot hook not found: $BOOT_HOOK" >&2
        exit 1
    fi
    if ! grep -q 'ZXTOOLS_BOOT' "$BOOT_HOOK" 2>/dev/null; then
        cp "$BOOT_HOOK" "$BOOT_HOOK.zxtools.bak" || exit 1
        {
            echo
            echo '# ZXTOOLS_BOOT'
            echo "[ -x \"$DEST/boot-links.sh\" ] && ZXTOOLS_HOME=\"$DEST\" ZXTOOLS_BIN=\"$LINK_DIR\" \"$DEST/boot-links.sh\""
        } >> "$BOOT_HOOK" || exit 1
    fi
fi

echo "installed: $DEST"
if [ "$NO_LINKS" = 1 ]; then
    echo "commands: $DEST/zxtools $DEST/router-check $DEST/fileinfo $DEST/zxnetmon (no links)"
else
    echo "commands: $LINK_DIR/zxtools $LINK_DIR/router-check $LINK_DIR/fileinfo $LINK_DIR/zxnetmon"
fi
