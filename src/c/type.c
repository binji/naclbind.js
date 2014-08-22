/* Copyright 2014 Ben Smith. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* DO NOT EDIT, this file is auto-generated from //templates/type.c */


#include <stdlib.h>

#include "type.h"

static const char* kTypeString[] = {
  "(null)",
  /* 1 */ "void",
  /* 2 */ "char",
  /* 3 */ "int8_t",
  /* 4 */ "uint8_t",
  /* 5 */ "int16_t",
  /* 6 */ "uint16_t",
  /* 7 */ "int32_t",
  /* 8 */ "uint32_t",
  /* 9 */ "long",
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
  /* 30 */ "struct PP_Var",
  /* 31 */ "struct PP_Var",
  /* 32 */ "struct PP_Var",
  /* 33 */ "struct PP_Var",
  /* 34 */ "struct PP_Var",
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
  /* 63 */ "void* (*)(void*, int32_t)",
  /* 64 */ "int32_t (*)(int32_t, int32_t)",
  /* 65 */ "uint32_t (*)(uint32_t, uint32_t)",
  /* 66 */ "int64_t (*)(int64_t, int64_t)",
  /* 67 */ "uint64_t (*)(uint64_t, uint64_t)",
  /* 68 */ "float (*)(float, float)",
  /* 69 */ "double (*)(double, double)",
  /* 70 */ "int (*)(void*, void*)",
  /* 71 */ "int (*)(char, char)",
  /* 72 */ "int (*)(int8_t, int8_t)",
  /* 73 */ "int (*)(uint8_t, uint8_t)",
  /* 74 */ "int (*)(int16_t, int16_t)",
  /* 75 */ "int (*)(uint16_t, uint16_t)",
  /* 76 */ "int (*)(uint32_t, uint32_t)",
  /* 77 */ "int (*)(long, long)",
  /* 78 */ "int (*)(unsigned long, unsigned long)",
  /* 79 */ "int (*)(int64_t, int64_t)",
  /* 80 */ "int (*)(uint64_t, uint64_t)",
  /* 81 */ "int (*)(float, float)",
  /* 82 */ "int (*)(double, double)",
  /* 83 */ "void (*)(void*)",
  /* 84 */ "void* (*)(size_t)",
  /* 85 */ "void* (*)(void*, int, size_t)",
  /* 86 */ "void* (*)(void*, void*, size_t)",
  /* 87 */ "size_t (*)(char*)",
  /* 88 */ "int (*)(char*)",
  /* 89 */ "void (*)(struct PP_Var)",
  /* 90 */ "struct PP_Var (*)(char*, uint32_t)",
  /* 91 */ "char* (*)(struct PP_Var, uint32_t*)",
  /* 92 */ "struct PP_Var (*)()",
  /* 93 */ "struct PP_Var (*)(struct PP_Var, uint32_t)",
  /* 94 */ "int32_t (*)(struct PP_Var, uint32_t, struct PP_Var)",
  /* 95 */ "uint32_t (*)(struct PP_Var)",
  /* 96 */ "int32_t (*)(struct PP_Var, uint32_t)",
  /* 97 */ "struct PP_Var (*)(uint32_t)",
  /* 98 */ "int32_t (*)(struct PP_Var, uint32_t*)",
  /* 99 */ "void* (*)(struct PP_Var)",
  /* 100 */ "void (*)(struct PP_Var)",
  /* 101 */ "struct PP_Var (*)()",
  /* 102 */ "struct PP_Var (*)(struct PP_Var, struct PP_Var)",
  /* 103 */ "int32_t (*)(struct PP_Var, struct PP_Var, struct PP_Var)",
  /* 104 */ "void (*)(struct PP_Var, struct PP_Var)",
  /* 105 */ "int32_t (*)(struct PP_Var, struct PP_Var)",
};

const char* BuiltinTypeToString(Type id) {
  if (id <= 0 || id >= NUM_BUILTIN_TYPES) {
    return "<unknown>";
  }
  return kTypeString[id];
}
