#!/bin/sh
# Optical-Modem-Tool installer: modem, local user, or custom prefix.
set -u

SRC=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
ALL_TOOLS="zxtools router-check fileinfo zxnetmon"

MODE=
DEST=
BIN=
TOOLS_SPEC=${ZXTOOLS_TOOLS:-}
WITH_BOOT=0
NO_LINKS=0
FROM=${ZXTOOLS_FROM:-$SRC/bin}
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
  --tools LIST     install only these tools (default: from config.mk or all)
  --from DIR       directory containing built binaries (default: <src>/bin)
  --boot           append a boot hook (modem mode only)
  --no-links       do not create command symlinks
  -h, --help       show this help

Environment:
  ZXTOOLS_HOME      override install directory
  ZXTOOLS_BIN       override symlink directory
  ZXTOOLS_TOOLS     default --tools list
  ZXTOOLS_FROM      override binary directory
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
        --prefix=*)
            DEST=${1#*=}
            MODE=custom
            ;;
        --bin-dir)
            shift
            [ $# -gt 0 ] || { echo "install: --bin-dir needs DIR" >&2; exit 2; }
            BIN=$1
            ;;
        --bin-dir=*) BIN=${1#*=} ;;
        --tools)
            shift
            [ $# -gt 0 ] || { echo "install: --tools needs a list" >&2; exit 2; }
            TOOLS_SPEC=$1
            ;;
        --tools=*) TOOLS_SPEC=${1#*=} ;;
        --from)
            shift
            [ $# -gt 0 ] || { echo "install: --from needs DIR" >&2; exit 2; }
            FROM=$1
            ;;
        --from=*) FROM=${1#*=} ;;
        --boot) WITH_BOOT=1 ;;
        --no-links) NO_LINKS=1 ;;
        -h|--help) usage; exit 0 ;;
        *) echo "install: unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
    shift
done

if [ -z "$TOOLS_SPEC" ] && [ -f "$SRC/config.mk" ]; then
    TOOLS_SPEC=$(sed -n 's/^TOOLS[[:space:]]*:=[[:space:]]*//p' \
        "$SRC/config.mk" | head -n 1)
fi
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
                echo "install: unknown tool: $tool" >&2
                echo "install: valid tools: $ALL_TOOLS" >&2
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
        echo "install: no tools selected" >&2
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
if [ "$WITH_BOOT" = 1 ] && [ "$NO_LINKS" = 1 ]; then
    echo "install: --boot needs command links; remove --no-links" >&2
    exit 2
fi
if [ "$WITH_BOOT" = 1 ] && [ ! -f "$BOOT_HOOK" ]; then
    echo "install: boot hook not found: $BOOT_HOOK" >&2
    exit 1
fi
if [ ! -d "$FROM" ]; then
    echo "install: binary directory not found: $FROM" >&2
    echo "install: run make first, or pass --from DIR" >&2
    exit 1
fi

for tool in $SELECTED_TOOLS; do
    if [ ! -f "$FROM/$tool" ]; then
        echo "install: missing binary: $FROM/$tool" >&2
        echo "install: run make, or pass --tools with the tools you built" >&2
        exit 1
    fi
done

mkdir -p "$DEST" "$DEST/reports" || exit 1

for tool in $SELECTED_TOOLS; do
    cp "$FROM/$tool" "$DEST/$tool" || exit 1
    chmod 755 "$DEST/$tool" || exit 1
done

cp "$SRC/boot-links.sh" "$DEST/boot-links.sh" || exit 1
chmod 755 "$DEST/boot-links.sh"

LINK_DIR=$BIN
if [ "$NO_LINKS" = 0 ]; then
    if ! mkdir -p "$LINK_DIR" 2>/dev/null || [ ! -w "$LINK_DIR" ]; then
        LINK_DIR=$DEST/bin
        mkdir -p "$LINK_DIR" || exit 1
    fi
    for tool in $SELECTED_TOOLS; do
        ln -sf "$DEST/$tool" "$LINK_DIR/$tool" || exit 1
    done
fi

if [ "$WITH_BOOT" = 1 ]; then
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
    printf 'commands:'
    for tool in $SELECTED_TOOLS; do
        printf ' %s' "$DEST/$tool"
    done
    printf ' (no links)\n'
else
    printf 'commands:'
    for tool in $SELECTED_TOOLS; do
        printf ' %s' "$LINK_DIR/$tool"
    done
    printf '\n'
fi
