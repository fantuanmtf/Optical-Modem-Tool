#!/bin/sh
# Recreate command symlinks after a reboot (or skip quietly if not installed).
set -u

ALL_TOOLS="zxtools router-check fileinfo zxnetmon"
DEST=${ZXTOOLS_HOME:-/usr/data/zxtools}
BIN=${ZXTOOLS_BIN:-/usr/bin}
TOOLS_SPEC=${ZXTOOLS_TOOLS:-all}

while [ $# -gt 0 ]; do
    case "$1" in
        --tools)
            shift
            [ $# -gt 0 ] || exit 0
            TOOLS_SPEC=$1
            ;;
        --tools=*) TOOLS_SPEC=${1#*=} ;;
        *) exit 0 ;;
    esac
    shift
done

[ -d "$DEST" ] || exit 0

SELECTED=
if [ "$TOOLS_SPEC" = all ]; then
    SELECTED=$ALL_TOOLS
else
    old_ifs=$IFS
    IFS=' ,:'
    for tool in $TOOLS_SPEC; do
        case " $ALL_TOOLS " in
            *" $tool "*) ;;
            *) IFS=$old_ifs; exit 0 ;;
        esac
        SELECTED="$SELECTED $tool"
    done
    IFS=$old_ifs
    SELECTED=${SELECTED# }
fi

if ! mkdir -p "$BIN" 2>/dev/null || [ ! -w "$BIN" ]; then
    BIN=$DEST/bin
    mkdir -p "$BIN" 2>/dev/null || exit 0
fi

for tool in $SELECTED; do
    if [ -x "$DEST/$tool" ]; then
        ln -sf "$DEST/$tool" "$BIN/$tool" || exit 0
    fi
done

exit 0
