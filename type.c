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
  /*  0 */ "(null)",
  /*  1 */ "void",
  /*  2 */ "int8_t",
  /*  3 */ "uint8_t",
  /*  4 */ "int16_t",
  /*  5 */ "uint16_t",
  /*  6 */ "int32_t",
  /*  7 */ "uint32_t",
  /*  8 */ "int64_t",
  /*  9 */ "uint64_t",
  /* 10 */ "float",
  /* 11 */ "double",

  /* 12 */ "void*",
  /* 13 */ "int8_t*",
  /* 14 */ "uint8_t*",
  /* 15 */ "int16_t*",
  /* 16 */ "uint16_t*",
  /* 17 */ "int32_t*",
  /* 18 */ "uint32_t*",
  /* 19 */ "int64_t*",
  /* 20 */ "uint64_t*",
  /* 21 */ "float*",
  /* 22 */ "double*",
  /* 23 */ "void**",

  /* 24 */ "PP_Var",
  /* 25 */ "PP_VarArrayBuffer",
  /* 26 */ "PP_VarArray",
  /* 27 */ "PP_VarDictionary",

  // get
  /* 28 */ "void* (*)(void**)",
  /* 29 */ "int8_t (*)(int8_t*)",
  /* 30 */ "uint8_t (*)(uint8_t*)",
  /* 31 */ "int16_t (*)(int16_t*)",
  /* 32 */ "uint16_t (*)(uint16_t*)",
  /* 33 */ "int32_t (*)(int32_t*)",
  /* 34 */ "uint32_t (*)(uint32_t*)",
  /* 35 */ "int64_t (*)(int64_t*)",
  /* 36 */ "uint64_t (*)(uint64_t*)",
  /* 37 */ "float (*)(float*)",
  /* 38 */ "double (*)(double*)",

  // set
  /* 39 */ "void (*)(void**, void*)",
  /* 40 */ "void (*)(int8_t*, int8_t)",
  /* 41 */ "void (*)(uint8_t*, uint8_t)",
  /* 42 */ "void (*)(int16_t*, int16_t)",
  /* 43 */ "void (*)(uint16_t*, uint16_t)",
  /* 44 */ "void (*)(int32_t*, int32_t)",
  /* 45 */ "void (*)(uint32_t*, uint32_t)",
  /* 46 */ "void (*)(int64_t*, int64_t)",
  /* 47 */ "void (*)(uint64_t*, uint64_t)",
  /* 48 */ "void (*)(float*, float)",
  /* 49 */ "void (*)(double*, double)",

  /* 50 */ "void (*)(void*)",  // free
  /* 51 */ "void* (*)(uint32_t)",  // malloc
  /* 52 */ "void (*)(void*, int32_t, uint32_t)",  // memset
  /* 53 */ "void (*)(void*, void*, uint32_t)",  // memcpy

  /* 54 */ "void (*)(PP_Var)",  // PPB_Var.AddRef / PPB_Var.Release
  /* 55 */ "PP_VarArrayBuffer (*)(uint32_t)",  // PPB_VarArrayBuffer.Create
  /* 56 */ "void* (*)(PP_VarArrayBuffer)",  // PPB_VarArrayBuffer.Map
  /* 57 */ "void (*)(PP_VarArrayBuffer)",  // PPB_VarArrayBuffer.Unmap

  // binary operators: add, sub, etc.
  /* 58 */ "void* (*)(void*, int32_t)",

  /* 59 */ "int32_t (*)(int32_t, int32_t)",
  /* 60 */ "uint32_t (*)(uint32_t, uint32_t)",
  /* 61 */ "int64_t (*)(int64_t, int64_t)",
  /* 62 */ "uint64_t (*)(uint64_t, uint64_t)",
  /* 63 */ "float (*)(float, float)",
  /* 64 */ "double (*)(double, double)",

  /* EXAMPLE ONLY!
   * TODO(binji): Move this to another file?
   */
  /* 65 */ "z_stream",
  /* 66 */ "z_stream*",
  /* 67 */ "int (*)(z_stream*, int)",
  /* 68 */ "int (*)(Bytef*, uLongf*, Bytef*, uLong)",
  /* 69 */ "uLong (*)(uLong)",
};

const char* TypeToString(Type id) {
  if (id <= 0 || id >= NUM_TYPES) {
    return "<unknown>";
  }
  return kTypeString[id];
}
