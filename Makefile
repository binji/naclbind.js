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
