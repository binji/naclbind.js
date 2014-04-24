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

static const char* kTypeString[] = {
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
  /* 27 */ "PP_VarString",

  // get
  /* 29 */ "void* (*)(void**)",
  /* 30 */ "int8_t (*)(int8_t*)",
  /* 31 */ "uint8_t (*)(uint8_t*)",
  /* 32 */ "int16_t (*)(int16_t*)",
  /* 33 */ "uint16_t (*)(uint16_t*)",
  /* 34 */ "int32_t (*)(int32_t*)",
  /* 35 */ "uint32_t (*)(uint32_t*)",
  /* 36 */ "int64_t (*)(int64_t*)",
  /* 37 */ "uint64_t (*)(uint64_t*)",
  /* 38 */ "float (*)(float*)",
  /* 39 */ "double (*)(double*)",

  // set
  /* 40 */ "void (*)(void**, void*)",
  /* 41 */ "void (*)(int8_t*, int8_t)",
  /* 42 */ "void (*)(uint8_t*, uint8_t)",
  /* 43 */ "void (*)(int16_t*, int16_t)",
  /* 44 */ "void (*)(uint16_t*, uint16_t)",
  /* 45 */ "void (*)(int32_t*, int32_t)",
  /* 46 */ "void (*)(uint32_t*, uint32_t)",
  /* 47 */ "void (*)(int64_t*, int64_t)",
  /* 48 */ "void (*)(uint64_t*, uint64_t)",
  /* 49 */ "void (*)(float*, float)",
  /* 50 */ "void (*)(double*, double)",

  /* 51 */ "void (*)(void*)",  // free
  /* 52 */ "void* (*)(uint32_t)",  // malloc
  /* 53 */ "void (*)(void*, int32_t, uint32_t)",  // memset
  /* 54 */ "void (*)(void*, void*, uint32_t)",  // memcpy
  /* 55 */ "size_t (*)(const char*)",  // strlen

  /* 56 */ "void (*)(PP_Var)",  // PPB_Var.AddRef / PPB_Var.Release
  /* 57 */ "PP_Var (*)(const char*, uint32_t)",  // PPB_Var.VarFromUtf8
  /* 58 */ "const char* (*)(PP_Var, uint32_t*)",  // PPB_Var.VarToUtf8

  /* 59 */ "PP_VarArray (*)()",  // PPB_VarArray.Create
  /* 60 */ "PP_Var (*)(PP_VarArray, uint32_t)",  // PPB_VarArray.Get
  /* 61 */ "PP_Bool (*)(PP_VarArray, uint32_t, PP_Var)",  // PPB_VarArray.Set
  /* 62 */ "uint32_t (*)(PP_VarArray)",  // PPB_VarArray.GetLength
  /* 63 */ "PP_Bool (*)(PP_VarArray, uint32_t)",  // PPB_VarArray.SetLength

  /* 64 */ "PP_VarArrayBuffer (*)(uint32_t)",  // PPB_VarArrayBuffer.Create
  /* 65 */ "PP_Bool (*)(PP_VarArrayBuffer, uint32_t*)",  // PPB_VarArrayBuffer.ByteLength
  /* 66 */ "void* (*)(PP_VarArrayBuffer)",  // PPB_VarArrayBuffer.Map
  /* 67 */ "void (*)(PP_VarArrayBuffer)",  // PPB_VarArrayBuffer.Unmap

  /* 68 */ "PP_VarDictionary (*)()",  // PPB_VarDictionary.Create
  /* 69 */ "PP_Var (*)(PP_VarDictionary, PP_Var)",  // PPB_VarDictionary.Get
  /* 70 */ "PP_Bool (*)(PP_VarDictionary, PP_Var, PP_Var)",  // PPB_VarDictionary.Set
  /* 71 */ "void (*)(PP_VarDictionary, PP_Var)",  // PPB_VarDictionary.Delete
  /* 72 */ "PP_Bool (*)(PP_VarDictionary, PP_Var)",  // PPB_VarDictionary.HasKey

  // binary operators: add, sub, etc.
  /* 73 */ "void* (*)(void*, int32_t)",

  /* 74 */ "int32_t (*)(int32_t, int32_t)",
  /* 75 */ "uint32_t (*)(uint32_t, uint32_t)",
  /* 76 */ "int64_t (*)(int64_t, int64_t)",
  /* 77 */ "uint64_t (*)(uint64_t, uint64_t)",
  /* 78 */ "float (*)(float, float)",
  /* 79 */ "double (*)(double, double)",
};

const char* BuiltinTypeToString(Type id) {
  if (id <= 0 || id >= NUM_BUILTIN_TYPES) {
    return "<unknown>";
  }
  return kTypeString[id];
}
