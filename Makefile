SHELL := /bin/sh
VERSION ?= 1.3.0
DIST ?= bin

-include config.mk

# Defaults keep "make clean" and "make package" useful before configure.
TOOLS ?=
BUILD_DIR ?= bin
OBJDIR ?= bin/obj
CC ?= cc
CPPFLAGS ?=
CFLAGS ?= -Os -std=c99 -Wall -Wextra
LDFLAGS ?=
LDLIBS ?=
DEFS ?=
HOSTCC ?= cc
HOST_CFLAGS ?= -O2 -g -std=c99 -Wall -Wextra
HOST_LDFLAGS ?=
HOST_DEFS ?=
PREFIX ?= /usr/data/zxtools
BINDIR ?= /usr/bin

COMMON_SRC := src/common/util.c src/common/compat.c
FILEINFO_SRC := src/fileinfo/main.c src/fileinfo/inspect.c
ZXNETMON_SRC := src/zxnetmon/main.c src/zxnetmon/netdev.c \
                src/zxnetmon/render.c src/zxnetmon/ui.c
ROUTERCHECK_SRC := src/router-check/main.c src/router-check/system.c \
                   src/router-check/memory.c src/router-check/storage.c \
                   src/router-check/netinfo.c src/router-check/report.c
ZXTOOLS_SRC := src/zxtools/main.c src/zxtools/menu.c src/zxtools/run.c

COMMON_OBJ := $(patsubst src/%.c,$(OBJDIR)/%.o,$(COMMON_SRC))
FILEINFO_OBJ := $(patsubst src/%.c,$(OBJDIR)/%.o,$(FILEINFO_SRC)) $(COMMON_OBJ)
ZXNETMON_OBJ := $(patsubst src/%.c,$(OBJDIR)/%.o,$(ZXNETMON_SRC)) $(COMMON_OBJ)
ROUTERCHECK_OBJ := $(patsubst src/%.c,$(OBJDIR)/%.o,$(ROUTERCHECK_SRC)) $(COMMON_OBJ)
ZXTOOLS_OBJ := $(patsubst src/%.c,$(OBJDIR)/%.o,$(ZXTOOLS_SRC)) $(COMMON_OBJ)
ALL_OBJ := $(sort $(COMMON_OBJ) $(FILEINFO_OBJ) $(ZXNETMON_OBJ) \
                   $(ROUTERCHECK_OBJ) $(ZXTOOLS_OBJ))

ALL_BIN := $(addprefix $(BUILD_DIR)/,$(TOOLS))

ifeq ($(HOST_KIND),native)
HOST_BIN_DIR := $(BUILD_DIR)
HOST_OBJDIR := $(OBJDIR)
HOST_BINARIES := $(ALL_BIN)
HOST_DEPS :=
else
HOST_BIN_DIR := bin/host
HOST_OBJDIR := bin/obj/host-check
HOST_COMMON_OBJ := $(patsubst src/%.c,$(HOST_OBJDIR)/%.o,$(COMMON_SRC))
HOST_FILEINFO_OBJ := $(patsubst src/%.c,$(HOST_OBJDIR)/%.o,$(FILEINFO_SRC)) \
                     $(HOST_COMMON_OBJ)
HOST_ZXNETMON_OBJ := $(patsubst src/%.c,$(HOST_OBJDIR)/%.o,$(ZXNETMON_SRC)) \
                     $(HOST_COMMON_OBJ)
HOST_ROUTERCHECK_OBJ := $(patsubst src/%.c,$(HOST_OBJDIR)/%.o,$(ROUTERCHECK_SRC)) \
                        $(HOST_COMMON_OBJ)
HOST_ZXTOOLS_OBJ := $(patsubst src/%.c,$(HOST_OBJDIR)/%.o,$(ZXTOOLS_SRC)) \
                    $(HOST_COMMON_OBJ)
HOST_ALL_OBJ := $(sort $(HOST_COMMON_OBJ) $(HOST_FILEINFO_OBJ) \
                       $(HOST_ZXNETMON_OBJ) $(HOST_ROUTERCHECK_OBJ) \
                       $(HOST_ZXTOOLS_OBJ))
HOST_BINARIES := $(addprefix $(HOST_BIN_DIR)/,$(TOOLS))
HOST_DEPS := $(HOST_ALL_OBJ:.o=.d)
endif

DEPS := $(sort $(ALL_OBJ:.o=.d) $(HOST_DEPS))

.PHONY: all arm check test install uninstall package clean distclean \
        require-config fileinfo zxnetmon router-check zxtools

all: require-config $(ALL_BIN)

arm: all

require-config:
	@if [ ! -f config.mk ]; then \
		echo "Makefile: config.mk not found; run ./configure first." >&2; \
		exit 1; \
	fi

$(OBJDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEFS) -Isrc/include -MMD -MP -c $< -o $@

$(BUILD_DIR)/fileinfo: $(FILEINFO_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/zxnetmon: $(ZXNETMON_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/router-check: $(ROUTERCHECK_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD_DIR)/zxtools: $(ZXTOOLS_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

fileinfo: require-config $(BUILD_DIR)/fileinfo
zxnetmon: require-config $(BUILD_DIR)/zxnetmon
router-check: require-config $(BUILD_DIR)/router-check
zxtools: require-config $(BUILD_DIR)/zxtools

ifneq ($(HOST_KIND),native)
$(HOST_OBJDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_DEFS) -Isrc/include -MMD -MP -c $< -o $@

$(HOST_BIN_DIR)/fileinfo: $(HOST_FILEINFO_OBJ)
	@mkdir -p $(HOST_BIN_DIR)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_LDFLAGS) -o $@ $^

$(HOST_BIN_DIR)/zxnetmon: $(HOST_ZXNETMON_OBJ)
	@mkdir -p $(HOST_BIN_DIR)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_LDFLAGS) -o $@ $^

$(HOST_BIN_DIR)/router-check: $(HOST_ROUTERCHECK_OBJ)
	@mkdir -p $(HOST_BIN_DIR)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_LDFLAGS) -o $@ $^

$(HOST_BIN_DIR)/zxtools: $(HOST_ZXTOOLS_OBJ)
	@mkdir -p $(HOST_BIN_DIR)
	$(HOSTCC) $(HOST_CFLAGS) $(HOST_LDFLAGS) -o $@ $^
endif

check: require-config $(HOST_BINARIES)
	@ZX_BIN_DIR="$(HOST_BIN_DIR)" ZX_TOOLS="$(TOOLS)" sh tests/smoke.sh

test: check

install: all
	@sh install.sh --prefix "$(PREFIX)" --bin-dir "$(BINDIR)" \
		--from "$(BUILD_DIR)" --tools "$(TOOLS)"

uninstall: require-config
	@sh uninstall.sh --prefix "$(PREFIX)" --bin-dir "$(BINDIR)" \
		--tools "$(TOOLS)"

package:
	@mkdir -p "$(DIST)"
	@tar czf "$(DIST)/optical-modem-tool-$(VERSION).tar.gz" \
		--exclude=./.git --exclude=./bin --exclude=./config.mk .
	@printf 'package: %s\n' "$(DIST)/optical-modem-tool-$(VERSION).tar.gz"

clean:
	rm -rf bin

distclean: clean
	rm -f config.mk

-include $(DEPS)
