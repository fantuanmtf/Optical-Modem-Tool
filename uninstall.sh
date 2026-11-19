#!/bin/sh
# Remove files installed by Optical-Modem-Tool/install.sh.
set -u

SRC=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
ALL_TOOLS="zxtools router-check fileinfo zxnetmon"

MODE=
DEST=
BIN=
TOOLS_SPEC=
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
  --tools LIST     remove only these tools (default: all)
  --keep-boot      keep the ZXTOOLS_BOOT block in the boot hook
  -h, --help       show this help

Environment:
  ZXTOOLS_HOME, ZXTOOLS_BIN, ZXTOOLS_BOOT_HOOK
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
        --prefix=*)
            DEST=${1#*=}
            MODE=custom
            ;;
        --bin-dir)
            shift
            [ $# -gt 0 ] || { echo "uninstall: --bin-dir needs DIR" >&2; exit 2; }
            BIN=$1
            ;;
        --bin-dir=*) BIN=${1#*=} ;;
        --tools)
            shift
            [ $# -gt 0 ] || { echo "uninstall: --tools needs a list" >&2; exit 2; }
            TOOLS_SPEC=$1
            ;;
        --tools=*) TOOLS_SPEC=${1#*=} ;;
        --keep-boot) KEEP_BOOT=1 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "uninstall: unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
    shift
done

[ -n "$TOOLS_SPEC" ] || TOOLS_SPEC=all

SELECTED_TOOLS=
normalize_tools() {
    spec=$1
    if [ "$spec" = all ]; then
        SELECTED_TOOLS=$ALL_TOOLS
        return 0
    fi
    SELECTED_TOOLS=
    old_ifs=$IFS
    IFS=' ,:'
    for tool in $spec; do
        case " $ALL_TOOLS " in
            *" $tool "*) ;;
            *)
                IFS=$old_ifs
                echo "uninstall: unknown tool: $tool" >&2
                exit 2
                ;;
        esac
        case " $SELECTED_TOOLS " in
            *" $tool "*) continue ;;
        esac
        SELECTED_TOOLS="$SELECTED_TOOLS $tool"
    done
    IFS=$old_ifs
    SELECTED_TOOLS=${SELECTED_TOOLS# }
    if [ -z "$SELECTED_TOOLS" ]; then
        echo "uninstall: no tools selected" >&2
        exit 2
    fi
}
normalize_tools "$TOOLS_SPEC"

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

for tool in $SELECTED_TOOLS; do
    for linkdir in "$BIN" "$DEST/bin"; do
        if [ -L "$linkdir/$tool" ]; then
            target=$(readlink "$linkdir/$tool" 2>/dev/null) || continue
            case "$target" in
                "$DEST/$tool"|"$DEST/bin/$tool") rm -f "$linkdir/$tool" ;;
            esac
        fi
    done
done

FULL=1
for tool in $ALL_TOOLS; do
    case " $SELECTED_TOOLS " in
        *" $tool "*) ;;
        *) FULL=0 ;;
    esac
done

if [ "$FULL" = 1 ]; then
    if [ "$KEEP_BOOT" = 0 ] && [ -f "$BOOT_HOOK" ] && \
       grep -q 'ZXTOOLS_BOOT' "$BOOT_HOOK" 2>/dev/null; then
        cp "$BOOT_HOOK" "$BOOT_HOOK.zxtools.uninstall.bak" || exit 1
        sed -i -e '/# ZXTOOLS_BOOT/d' -e '/boot-links.sh/d' "$BOOT_HOOK" || exit 1
    fi
    if [ -n "$DEST" ] && [ -d "$DEST" ]; then
        rm -rf "$DEST" || exit 1
    fi
    echo "uninstalled: $DEST"
    [ "$KEEP_BOOT" = 1 ] && echo "kept boot hook block"
else
    for tool in $SELECTED_TOOLS; do
        rm -f "$DEST/$tool"
    done
    echo "removed tools from: $DEST"
    echo "selected: $SELECTED_TOOLS"
    echo "kept boot hook block (partial uninstall)"
fi
exit 0
