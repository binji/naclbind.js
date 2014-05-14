# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# GNU Makefile based on shared rules provided by the Native Client SDK.
# See README.Makefiles for more details.

VALID_TOOLCHAINS := pnacl
LIBDIR := $(CURDIR)
EXTRA_LIB_PATHS = $(CURDIR)

ifeq (,$(NACL_SDK_ROOT))
  $(error NACL_SDK_ROOT is not set.)
endif
include $(NACL_SDK_ROOT)/tools/common.mk

## Rules to build zlib from naclports ##########################################
PORTS = zlib

.PHONY: ports
ports:
ifeq (newlib,$(TOOLCHAIN))
	$(MAKE) -C third_party/naclports $(PORTS) NACL_ARCH=i686 FORCE=1
	$(MAKE) -C third_party/naclports $(PORTS) NACL_ARCH=x86_64 FORCE=1
	$(MAKE) -C third_party/naclports $(PORTS) NACL_ARCH=arm FORCE=1
endif
ifeq (pnacl,$(TOOLCHAIN))
	$(MAKE) -C third_party/naclports $(PORTS) NACL_ARCH=pnacl FORCE=1
endif

## Rules to build 2nacl library and pexes that use it #########################
TARGETS = zlib zip
2nacl_SOURCES =\
  2nacl/commands.c \
  2nacl/handle.c \
  2nacl/interfaces.c \
  2nacl/message.c \
  2nacl/queue.c \
  2nacl/type.c \
  2nacl/var.c \

# TODO(binji): it sucks to have to duplicate app.c for zlib/zip, but I'm not
# sure of a better way using the SDK build system. I'd like to use the same .c
# file, but there will be multiple rules to generate app.o in that case.
zlib_SOURCES =\
  zlib/zlib_app.c \
  zlib/zlib_commands.c \
  zlib/zlib_type.c \

zip_SOURCES =\
  zip/ioapi.c \
  zip/zip_app.c \
  zip/zip.c \
  zip/zip_commands.c \
  zip/zip_type.c \

CFLAGS += -Wall -DUSE_FILE32API -DNOCRYPT -Wno-unused-value -I$(CURDIR)/2nacl
LIBS = 2nacl nacl_io z ppapi_cpp ppapi

EASY_TEMPLATE = py/easy_template.py
ZIP_JSON = py/zip.json
ZLIB_JSON = py/zlib.json

# Zip generated files
zip/zip_type.h: py/type.h.template $(EASY_TEMPLATE) $(ZIP_JSON)
	$(call LOG,TEMPLATE,$@,$(EASY_TEMPLATE) -j $(ZIP_JSON) $< > $@)

zip/zip_type.c: py/type.c.template $(EASY_TEMPLATE) $(ZIP_JSON)
	$(call LOG,TEMPLATE,$@,$(EASY_TEMPLATE) -j $(ZIP_JSON) $< > $@)

zip/zip_commands.h: py/commands.h.template $(EASY_TEMPLATE) $(ZIP_JSON)
	$(call LOG,TEMPLATE,$@,$(EASY_TEMPLATE) -j $(ZIP_JSON) $< > $@)

zip/zip_commands.c: py/commands.c.template $(EASY_TEMPLATE) $(ZIP_JSON)
	$(call LOG,TEMPLATE,$@,$(EASY_TEMPLATE) -j $(ZIP_JSON) $< > $@)

all: js/zip_gen.js
js/zip_gen.js: py/gen.js.template $(EASY_TEMPLATE) $(ZIP_JSON)
	$(call LOG,TEMPLATE,$@,$(EASY_TEMPLATE) -j $(ZIP_JSON) $< > $@)


# Zlib generated files
zlib/zlib_type.h: py/type.h.template $(EASY_TEMPLATE) $(ZLIB_JSON)
	$(call LOG,TEMPLATE,$@,$(EASY_TEMPLATE) -j $(ZLIB_JSON) $< > $@)

zlib/zlib_type.c: py/type.c.template $(EASY_TEMPLATE) $(ZLIB_JSON)
	$(call LOG,TEMPLATE,$@,$(EASY_TEMPLATE) -j $(ZLIB_JSON) $< > $@)

zlib/zlib_commands.h: py/commands.h.template $(EASY_TEMPLATE) $(ZLIB_JSON)
	$(call LOG,TEMPLATE,$@,$(EASY_TEMPLATE) -j $(ZLIB_JSON) $< > $@)

zlib/zlib_commands.c: py/commands.c.template $(EASY_TEMPLATE) $(ZLIB_JSON)
	$(call LOG,TEMPLATE,$@,$(EASY_TEMPLATE) -j $(ZLIB_JSON) $< > $@)

all: js/zlib_gen.js
js/zlib_gen.js: py/gen.js.template $(EASY_TEMPLATE) $(ZLIB_JSON)
	$(call LOG,TEMPLATE,$@,$(EASY_TEMPLATE) -j $(ZLIB_JSON) $< > $@)


# Build lib2nacl.a
$(foreach src,$(2nacl_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CFLAGS))))
$(eval $(call LIB_RULE,2nacl,$(2nacl_SOURCES)))

# Build targets that use it.
define TARGET_RULE
pnacl/$(CONFIG)/$(1)_unstripped.bc: | $(LIBDIR)/pnacl/$(CONFIG)/lib2nacl.a

$$(foreach src,$$($(1)_SOURCES),$$(eval $$(call COMPILE_RULE,$$(src),$(CFLAGS))))
$$(eval $$(call LINK_RULE,$(1)_unstripped,$$($(1)_SOURCES),$(LIBS)))
$$(eval $$(call STRIP_RULE,$(1),$(1)_unstripped))
$$(eval $$(call NMF_RULE,$(1),))
endef

$(foreach target,$(TARGETS),$(eval $(call TARGET_RULE,$(target))))
