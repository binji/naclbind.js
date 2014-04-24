# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# GNU Makefile based on shared rules provided by the Native Client SDK.
# See README.Makefiles for more details.

VALID_TOOLCHAINS := pnacl
LIBDIR := $(PWD)
EXTRA_LIB_PATHS = $(PWD)

ifeq (,$(NACL_SDK_ROOT))
  $(error NACL_SDK_ROOT is not set.)
endif
include $(NACL_SDK_ROOT)/tools/common.mk

## Rules to build zlib from naclports ##########################################
PORTS = zlib

.PHONY: ports
ports:
ifeq (newlib,$(TOOLCHAIN))
	$(MAKE) -C third_party/naclports $(PORTS) NACL_ARCH=i686
	$(MAKE) -C third_party/naclports $(PORTS) NACL_ARCH=x86_64
	$(MAKE) -C third_party/naclports $(PORTS) NACL_ARCH=arm
endif
ifeq (pnacl,$(TOOLCHAIN))
	$(MAKE) -C third_party/naclports $(PORTS) NACL_ARCH=pnacl
endif

## Rules to build jsnacl library and pexes that use it #########################
TARGETS = zlib zip
jsnacl_SOURCES =\
  commands.c \
  handle.c \
  interfaces.c \
  message.c \
  queue.c \
  type.c \
  var.c \

# TODO(binji): it sucks to have to duplicate app.c for zlib/zip, but I'm not
# sure of a better way using the SDK build system. I'd like to use the same .c
# file, but there will be multiple rules to generate app.o in that case.
zlib_SOURCES = zlib_app.c zlib_commands.c zlib_type.c
zip_SOURCES = zip_app.c ioapi.c zip.c zip_commands.c zip_type.c

CFLAGS += -Wall -DUSE_FILE32API -DNOCRYPT -Wno-unused-value
LIBS = jsnacl nacl_io z ppapi_cpp ppapi

# Build libjsnacl.a
$(foreach src,$(jsnacl_SOURCES),$(eval $(call COMPILE_RULE,$(src),$(CFLAGS))))
$(eval $(call LIB_RULE,jsnacl,$(jsnacl_SOURCES)))

# Build targets that use it.
define TARGET_RULE
pnacl/$(CONFIG)/$(1)_unstripped.pexe: | pnacl/Release/libjsnacl.a

$$(foreach src,$$($(1)_SOURCES),$$(eval $$(call COMPILE_RULE,$$(src),$(CFLAGS))))
$$(eval $$(call LINK_RULE,$(1)_unstripped,$$($(1)_SOURCES),$(LIBS)))
$$(eval $$(call STRIP_RULE,$(1),$(1)_unstripped))
$$(eval $$(call NMF_RULE,$(1),))
endef

$(foreach target,$(TARGETS),$(eval $(call TARGET_RULE,$(target))))
