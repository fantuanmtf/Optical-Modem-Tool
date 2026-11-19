SHELL := /bin/sh
VERSION ?= 1.1.0
PREFIX ?= /usr/data/zxtools
BINDIR ?= /usr/bin
DIST ?= bin

.PHONY: all check test install uninstall package clean

all: check

check:
	@set -e; \
	for f in \
		src/zxtools/zxtools \
		src/router-check/router-check \
		src/fileinfo/fileinfo \
		src/zxnetmon/zxnetmon \
		install.sh uninstall.sh boot-links.sh tests/smoke.sh; do \
		sh -n "$$f"; \
	done
	@sh tests/smoke.sh

test: check

install: check
	@sh install.sh --prefix "$(PREFIX)" --bin-dir "$(BINDIR)"

uninstall:
	@sh uninstall.sh --prefix "$(PREFIX)" --bin-dir "$(BINDIR)"

package: check
	@mkdir -p "$(DIST)"
	@tar czf "$(DIST)/optical-modem-tool-$(VERSION).tar.gz" \
		--exclude=.git --exclude="$(DIST)" --exclude='*.log' --exclude='*.tmp' .
	@printf 'package: %s\n' "$(DIST)/optical-modem-tool-$(VERSION).tar.gz"

clean:
	rm -rf "$(DIST)"
