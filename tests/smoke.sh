#!/bin/sh
# Host-side smoke tests for the shell toolkit.
set -u

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SRC="$ROOT/src"
TMP="${TMPDIR:-/tmp}/zxtools-smoke.$$"
FAIL=0

cleanup() {
    rm -f "$TMP"
}
trap cleanup EXIT INT TERM

pass() {
    printf 'PASS %s\n' "$1"
}

fail() {
    printf 'FAIL %s\n' "$1"
    FAIL=$((FAIL + 1))
}

check_syntax() {
    if sh -n "$1"; then
        pass "syntax $1"
    else
        fail "syntax $1"
    fi
}

for f in \
    "$SRC/zxtools/zxtools" \
    "$SRC/router-check/router-check" \
    "$SRC/fileinfo/fileinfo" \
    "$SRC/zxnetmon/zxnetmon" \
    "$ROOT/install.sh" "$ROOT/uninstall.sh" "$ROOT/boot-links.sh" \
    "$ROOT/tests/smoke.sh"; do
    check_syntax "$f"
done

if OUT=$("$SRC/fileinfo/fileinfo" "$ROOT/README.md" 2>&1); then
    case "$OUT" in
        *"Path:"*"$ROOT/README.md"*) pass "fileinfo runs" ;;
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
    MOD=$(printf '%s\n' "$OUT" | awk -F': ' '/^Modified:/{print $2}')
    case "$MOD" in
        *README.md*) fail "fileinfo mtime includes filename" ;;
        *) pass "fileinfo mtime excludes filename" ;;
    esac
else
    fail "fileinfo runs"
fi

if "$SRC/router-check/router-check" -o "$TMP" >/dev/null 2>&1; then
    for key in System Memory Storage; do
        if grep -q "$key" "$TMP"; then
            pass "router-check $key"
        else
            fail "router-check $key"
        fi
    done
else
    fail "router-check runs"
fi

if OUT=$("$SRC/zxtools/zxtools" help 2>&1); then
    case "$OUT" in
        *Usage:*) pass "zxtools help" ;;
        *) fail "zxtools help output" ;;
    esac
else
    fail "zxtools help"
fi

if command -v timeout >/dev/null 2>&1; then
    OUT=$(timeout 2 "$SRC/zxnetmon/zxnetmon" -i lo -n 1 2>&1 || true)
else
    OUT=$("$SRC/zxnetmon/zxnetmon" -i lo -n 1 2>&1 & ZPID=$!; sleep 2; kill "$ZPID" 2>/dev/null; wait "$ZPID" 2>/dev/null || true)
fi
case "$OUT" in
    *"ZX NETMONITOR"*) pass "zxnetmon runs" ;;
    *) fail "zxnetmon runs" ;;
esac
case "$OUT" in
    *"Press Ctrl+C"*) pass "zxnetmon header" ;;
    *) fail "zxnetmon header" ;;
esac

if [ "$FAIL" -ne 0 ]; then
    printf '%s smoke test(s) failed.\n' "$FAIL"
    exit 1
fi
printf 'All smoke tests passed.\n'
