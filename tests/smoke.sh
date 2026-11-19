#!/bin/sh
# Host-side smoke tests for the C toolkit.
#
# make check sets ZX_BIN_DIR=bin/host and ZX_TOOLS to the configured list.
set -u

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 1
BIN=${ZX_BIN_DIR:-$ROOT/bin/host}
TOOLS_SPEC=${ZX_TOOLS:-all}
ALL_TOOLS="fileinfo zxnetmon router-check zxtools"
TMP="${TMPDIR:-/tmp}/zxtools-smoke.$$"
FAIL=0

cleanup() {
    rm -rf "$TMP"
}
trap cleanup EXIT INT TERM

pass() {
    printf 'PASS %s\n' "$1"
}

fail() {
    printf 'FAIL %s\n' "$1"
    FAIL=$((FAIL + 1))
}

if [ "$TOOLS_SPEC" = all ]; then
    SELECTED=$ALL_TOOLS
else
    SELECTED=
    old_ifs=$IFS
    IFS=' ,:'
    for tool in $TOOLS_SPEC; do
        SELECTED="$SELECTED $tool"
    done
    IFS=$old_ifs
    SELECTED=${SELECTED# }
fi

has_tool() {
    case " $SELECTED " in
        *" $1 "*) return 0 ;;
    esac
    return 1
}

mkdir -p "$TMP" || exit 1

printf 'smoke: binaries %s\n' "$BIN"
printf 'smoke: tools    %s\n' "$SELECTED"

for tool in $SELECTED; do
    if [ -x "$BIN/$tool" ]; then
        pass "binary $tool"
    else
        fail "binary $tool missing or not executable"
    fi
done

if has_tool fileinfo && [ -x "$BIN/fileinfo" ]; then
    if OUT=$("$BIN/fileinfo" "$ROOT/README.md" 2>&1); then
        pass "fileinfo runs"
        case "$OUT" in
            *"Path:"*"$ROOT/README.md"*) pass "fileinfo path output" ;;
            *) fail "fileinfo path output" ;;
        esac
        case "$OUT" in
            *"regular file"*) pass "fileinfo type" ;;
            *) fail "fileinfo type output" ;;
        esac
        case "$OUT" in
            *"Size:"*) pass "fileinfo size" ;;
            *) fail "fileinfo size output" ;;
        esac
        MOD=$(printf '%s\n' "$OUT" | sed -n 's/^Modified: //p')
        case "$MOD" in
            "") fail "fileinfo mtime missing" ;;
            *README.md*) fail "fileinfo mtime includes filename" ;;
            *) pass "fileinfo mtime excludes filename" ;;
        esac
    else
        fail "fileinfo runs"
    fi
fi

if has_tool router-check && [ -x "$BIN/router-check" ]; then
    if "$BIN/router-check" -o "$TMP/report.txt" \
            > "$TMP/report-stdout.txt" 2>&1; then
        for key in System Memory Storage; do
            if grep -q "== $key ==" "$TMP/report.txt"; then
                pass "router-check $key file"
            else
                fail "router-check $key file"
            fi
            if grep -q "== $key ==" "$TMP/report-stdout.txt"; then
                pass "router-check $key stdout"
            else
                fail "router-check $key stdout"
            fi
        done
    else
        fail "router-check runs"
    fi
fi

if has_tool zxtools && [ -x "$BIN/zxtools" ]; then
    if OUT=$("$BIN/zxtools" help 2>&1); then
        case "$OUT" in
            *"Usage:"*) pass "zxtools help" ;;
            *) fail "zxtools help output" ;;
        esac
    else
        fail "zxtools help"
    fi

    if OUT=$("$BIN/zxtools" < /dev/null 2>&1); then
        case "$OUT" in
            *"Usage:"*) pass "zxtools non-TTY usage" ;;
            *) fail "zxtools non-TTY usage" ;;
        esac
    else
        fail "zxtools non-TTY exit"
    fi

    if has_tool fileinfo && [ -x "$BIN/fileinfo" ]; then
        if OUT=$("$BIN/zxtools" fileinfo "$ROOT/README.md" 2>&1); then
            case "$OUT" in
                *"Path:"*"$ROOT/README.md"*) pass "zxtools dispatch" ;;
                *) fail "zxtools dispatch output" ;;
            esac
        else
            fail "zxtools dispatch"
        fi
    fi
fi

if has_tool zxnetmon && [ -x "$BIN/zxnetmon" ]; then
    if command -v timeout >/dev/null 2>&1; then
        OUT=$(timeout 2 "$BIN/zxnetmon" -i lo -n 1 2>&1 || true)
    else
        "$BIN/zxnetmon" -i lo -n 1 > "$TMP/netmon.out" 2>&1 &
        ZPID=$!
        sleep 2
        kill "$ZPID" 2>/dev/null
        wait "$ZPID" 2>/dev/null || true
        OUT=$(cat "$TMP/netmon.out")
    fi
    case "$OUT" in
        *"ZX NETMONITOR"*) pass "zxnetmon runs" ;;
        *) fail "zxnetmon runs" ;;
    esac
    case "$OUT" in
        *"Press Ctrl+C"*) pass "zxnetmon header" ;;
        *) fail "zxnetmon header" ;;
    esac

    if OUT=$("$BIN/zxnetmon" -i lo --list 2>&1); then
        case "$OUT" in
            lo) pass "zxnetmon list mode" ;;
            *) fail "zxnetmon list mode output" ;;
        esac
    else
        fail "zxnetmon list mode"
    fi
fi

if [ "$FAIL" -ne 0 ]; then
    printf '%s smoke test(s) failed.\n' "$FAIL"
    exit 1
fi
printf 'All smoke tests passed.\n'
