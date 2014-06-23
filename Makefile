# Copyright 2014 Ben Smith. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

## Rules to build zlib from naclports ##########################################
TOOLCHAIN = pnacl
PORTS = zlib
NACL_SDK_ROOT = $(CURDIR)/out/nacl_sdk/pepper_35

.PHONY: ports
ports:
ifeq (newlib,$(TOOLCHAIN))
	NACL_SDK_ROOT=$(NACL_SDK_ROOT) $(MAKE) -C third_party/naclports $(PORTS) NACL_ARCH=i686 FORCE=1
	NACL_SDK_ROOT=$(NACL_SDK_ROOT) $(MAKE) -C third_party/naclports $(PORTS) NACL_ARCH=x86_64 FORCE=1
	NACL_SDK_ROOT=$(NACL_SDK_ROOT) $(MAKE) -C third_party/naclports $(PORTS) NACL_ARCH=arm FORCE=1
endif
ifeq (pnacl,$(TOOLCHAIN))
	NACL_SDK_ROOT=$(NACL_SDK_ROOT) $(MAKE) -C third_party/naclports $(PORTS) NACL_ARCH=pnacl FORCE=1
endif

out/nacl_sdk.zip:
	cd out && \
	wget http://storage.googleapis.com/nativeclient-mirror/nacl/nacl_sdk/nacl_sdk.zip

out/nacl_sdk: out/nacl_sdk.zip
	cd out && \
	unzip nacl_sdk.zip

.PHONY: sdk
sdk: | out/nacl_sdk
	out/nacl_sdk/naclsdk update pepper_35 --force
