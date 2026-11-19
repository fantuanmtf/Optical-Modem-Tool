#!/bin/sh
# Smoke tests for the shell-only Optical-Modem-Tool suite.
# Run from anywhere: sh tests/smoke.sh
set -u

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP="/tmp/zxtools-smoke.$$"
FAILED=0

cleanup() {
    rm -f "$TMP"
}
trap cleanup EXIT HUP INT TERM

pass() {
    printf 'PASS  %s\n' "$1"
}

fail() {
    printf 'FAIL  %s\n' "$1"
    FAILED=1
}

check_syntax() {
    f=$1
    if sh -n "$ROOT/$f"; then
        pass "syntax $f"
    else
        fail "syntax $f"
    fi
}

check_syntax zxtools/zxtools
check_syntax router-check/router-check
check_syntax fileinfo/fileinfo
check_syntax zxnetmon/zxnetmon
check_syntax install.sh
check_syntax uninstall.sh
check_syntax boot-links.sh

if "$ROOT/fileinfo/fileinfo" "$ROOT/README.md" >"$TMP" 2>&1; then
    pass "fileinfo runs"
else
    fail "fileinfo runs"
fi
if grep -q '^Path:' "$TMP" && grep -q '^Size:' "$TMP" && grep -q '^Modified:' "$TMP"; then
    pass "fileinfo fields"
else
    fail "fileinfo fields"
fi
if grep '^Modified:' "$TMP" | grep -q 'README.md'; then
    fail "fileinfo mtime excludes filename"
else
    pass "fileinfo mtime excludes filename"
fi

if "$ROOT/router-check/router-check" -o "$TMP" >/dev/null 2>&1; then
    pass "router-check runs"
else
    fail "router-check runs"
fi
if grep -q 'System' "$TMP" && grep -q 'Memory' "$TMP" && grep -q 'Storage' "$TMP"; then
    pass "router-check sections"
else
    fail "router-check sections"
fi

if "$ROOT/zxtools/zxtools" help >"$TMP" 2>&1; then
    pass "zxtools help"
else
    fail "zxtools help"
fi
if grep -q 'Usage' "$TMP"; then
    pass "zxtools usage text"
else
    fail "zxtools usage text"
fi

if command -v timeout >/dev/null 2>&1; then
    timeout 2 "$ROOT/zxnetmon/zxnetmon" -i lo -n 1 >"$TMP" 2>&1
    rc=$?
    if [ "$rc" -eq 124 ] || [ "$rc" -eq 0 ]; then
        pass "zxnetmon runs"
    else
        fail "zxnetmon runs (rc=$rc)"
    fi
    if grep -q 'ZX NETMONITOR' "$TMP"; then
        pass "zxnetmon header"
    else
        fail "zxnetmon header"
    fi
else
    printf 'SKIP  zxnetmon (timeout not found)\n'
fi

if [ "$FAILED" -eq 0 ]; then
    printf '\nAll smoke tests passed.\n'
    exit 0
fi
printf '\nSome smoke tests failed.\n'
exit 1
