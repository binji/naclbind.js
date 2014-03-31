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

#include "type.h"

const char* kTypeString[] = {
  "void",
  "int8_t",
  "uint8_t",
  "int16_t",
  "uint16_t",
  "int32_t",
  "uint32_t",
  "int64_t",
  "uint64_t",
  "float",
  "double",
  "size_t",
  "void*",
  "uint8_t*",
  "uint8_t**",
  "uint32_t*",
  "pp::VarArrayBuffer",
  "pp::VarArray",
  "pp::VarDictionary",
  "void* (*)(size_t)",
  "void (*)(void*, int32, size_t)",
  "void (*)(void*, void*, size_t)",
  "void* (*)(void*, int32)",
  "void (*)(uint8_t**, uint8_t*)",
  "void (*)(uint32_t*, uint32_t)",
  "uint8_t* (*)(uint8_t**)",
  "uint32_t (*)(uint32_t*)",
  "int32 (*)(int32, int32)",
  "uint32 (*)(uint32, uint32)",
  "pp::VarArrayBuffer (*)(uint32)",
  "void* (*)(pp::VarArrayBuffer)",
  "void (*)(pp::VarArrayBuffer)",
  "z_stream",
  "z_stream*",
  "int (*)(z_stream*, int)",
};

const char* TypeToString(Type id) {
  if (id <= 0 || id >= NUM_TYPES) {
    return NULL;
  }
  return kTypeString[id];
}
