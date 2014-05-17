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
  /*  2 */ "char",
  /*  3 */ "int8_t",
  /*  4 */ "uint8_t",
  /*  5 */ "int16_t",
  /*  6 */ "uint16_t",
  /*  7 */ "int32_t",
  /*  8 */ "uint32_t",
  /*  9 */ "long",
  /* 10 */ "unsigned long",
  /* 11 */ "int64_t",
  /* 12 */ "uint64_t",
  /* 13 */ "float",
  /* 14 */ "double",

  /* 15 */ "void*",
  /* 16 */ "char*",
  /* 17 */ "int8_t*",
  /* 18 */ "uint8_t*",
  /* 19 */ "int16_t*",
  /* 20 */ "uint16_t*",
  /* 21 */ "int32_t*",
  /* 22 */ "uint32_t*",
  /* 23 */ "long*",
  /* 24 */ "unsigned long*",
  /* 25 */ "int64_t*",
  /* 26 */ "uint64_t*",
  /* 27 */ "float*",
  /* 28 */ "double*",
  /* 29 */ "void**",

  /* 30 */ "PP_Var",
  /* 31 */ "PP_VarArrayBuffer",
  /* 32 */ "PP_VarArray",
  /* 33 */ "PP_VarDictionary",
  /* 34 */ "PP_VarString",

  // get
  /* 35 */ "void* (*)(void**)",
  /* 36 */ "char (*)(char*)",
  /* 37 */ "int8_t (*)(int8_t*)",
  /* 38 */ "uint8_t (*)(uint8_t*)",
  /* 39 */ "int16_t (*)(int16_t*)",
  /* 40 */ "uint16_t (*)(uint16_t*)",
  /* 41 */ "int32_t (*)(int32_t*)",
  /* 42 */ "uint32_t (*)(uint32_t*)",
  /* 43 */ "long (*)(long*)",
  /* 44 */ "unsigned long (*)(unsigned long*)",
  /* 45 */ "int64_t (*)(int64_t*)",
  /* 46 */ "uint64_t (*)(uint64_t*)",
  /* 47 */ "float (*)(float*)",
  /* 48 */ "double (*)(double*)",

  // set
  /* 49 */ "void (*)(void**, void*)",
  /* 50 */ "void (*)(char*, char)",
  /* 51 */ "void (*)(int8_t*, int8_t)",
  /* 52 */ "void (*)(uint8_t*, uint8_t)",
  /* 53 */ "void (*)(int16_t*, int16_t)",
  /* 54 */ "void (*)(uint16_t*, uint16_t)",
  /* 55 */ "void (*)(int32_t*, int32_t)",
  /* 56 */ "void (*)(uint32_t*, uint32_t)",
  /* 57 */ "void (*)(long*, long)",
  /* 58 */ "void (*)(unsigned long*, unsigned long)",
  /* 59 */ "void (*)(int64_t*, int64_t)",
  /* 60 */ "void (*)(uint64_t*, uint64_t)",
  /* 61 */ "void (*)(float*, float)",
  /* 62 */ "void (*)(double*, double)",

  /* 63 */ "void (*)(void*)",  // free
  /* 64 */ "void* (*)(size_t)",  // malloc
  /* 65 */ "void (*)(void*, int, size_t)",  // memset
  /* 66 */ "void (*)(void*, void*, size_t)",  // memcpy
  /* 67 */ "size_t (*)(const char*)",  // strlen

  /* 68 */ "void (*)(PP_Var)",  // PPB_Var.AddRef / PPB_Var.Release
  /* 69 */ "PP_Var (*)(const char*, uint32_t)",  // PPB_Var.VarFromUtf8
  /* 70 */ "const char* (*)(PP_Var, uint32_t*)",  // PPB_Var.VarToUtf8

  /* 71 */ "PP_VarArray (*)()",  // PPB_VarArray.Create
  /* 72 */ "PP_Var (*)(PP_VarArray, uint32_t)",  // PPB_VarArray.Get
  /* 73 */ "PP_Bool (*)(PP_VarArray, uint32_t, PP_Var)",  // PPB_VarArray.Set
  /* 74 */ "uint32_t (*)(PP_VarArray)",  // PPB_VarArray.GetLength
  /* 75 */ "PP_Bool (*)(PP_VarArray, uint32_t)",  // PPB_VarArray.SetLength

  /* 76 */ "PP_VarArrayBuffer (*)(uint32_t)",  // PPB_VarArrayBuffer.Create
  /* 77 */ "PP_Bool (*)(PP_VarArrayBuffer, uint32_t*)",  // PPB_VarArrayBuffer.ByteLength
  /* 78 */ "void* (*)(PP_VarArrayBuffer)",  // PPB_VarArrayBuffer.Map
  /* 79 */ "void (*)(PP_VarArrayBuffer)",  // PPB_VarArrayBuffer.Unmap

  /* 80 */ "PP_VarDictionary (*)()",  // PPB_VarDictionary.Create
  /* 81 */ "PP_Var (*)(PP_VarDictionary, PP_Var)",  // PPB_VarDictionary.Get
  /* 82 */ "PP_Bool (*)(PP_VarDictionary, PP_Var, PP_Var)",  // PPB_VarDictionary.Set
  /* 83 */ "void (*)(PP_VarDictionary, PP_Var)",  // PPB_VarDictionary.Delete
  /* 84 */ "PP_Bool (*)(PP_VarDictionary, PP_Var)",  // PPB_VarDictionary.HasKey

  // binary operators: add, sub, etc.
  /* 85 */ "void* (*)(void*, int32_t)",

  /* 86 */ "int32_t (*)(int32_t, int32_t)",
  /* 87 */ "uint32_t (*)(uint32_t, uint32_t)",
  /* 88 */ "int64_t (*)(int64_t, int64_t)",
  /* 89 */ "uint64_t (*)(uint64_t, uint64_t)",
  /* 90 */ "float (*)(float, float)",
  /* 91 */ "double (*)(double, double)",
};

const char* BuiltinTypeToString(Type id) {
  if (id <= 0 || id >= NUM_BUILTIN_TYPES) {
    return "<unknown>";
  }
  return kTypeString[id];
}
