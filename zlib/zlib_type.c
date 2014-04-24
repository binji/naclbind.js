// Copyright 2014 Ben Smith. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdlib.h>

#include "zlib_type.h"

static const char* kTypeString[] = {
  /* 80 */ "z_stream",
  /* 81 */ "z_stream*",
  /* 82 */ "int (*)(z_stream*, int)",
  /* 83 */ "int (*)(Bytef*, uLongf*, Bytef*, uLong)",
  /* 84 */ "uLong (*)(uLong)",
  /* 85 */ "const char* (*)()",
};

const char* TypeToString(Type id) {
  if (id < NUM_BUILTIN_TYPES) {
    return BuiltinTypeToString(id);
  } else if (id >= NUM_TYPES) {
    return "<unknown>";
  } else {
    return kTypeString[id - NUM_BUILTIN_TYPES];
  }
}
