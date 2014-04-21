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

#ifndef TYPE_H_
#define TYPE_H_

typedef enum {
  TYPE_NONE = 0,
  TYPE_VOID = 1,
  TYPE_INT8 = 2,
  TYPE_UINT8 = 3,
  TYPE_INT16 = 4,
  TYPE_UINT16 = 5,
  TYPE_INT32 = 6,
  TYPE_UINT32 = 7,
  TYPE_INT64 = 8,
  TYPE_UINT64 = 9,
  TYPE_FLOAT = 10,
  TYPE_DOUBLE = 11,

  TYPE_VOID_P = 12,
  TYPE_INT8_P = 13,
  TYPE_UINT8_P = 14,
  TYPE_INT16_P = 15,
  TYPE_UINT16_P = 16,
  TYPE_INT32_P = 17,
  TYPE_UINT32_P = 18,
  TYPE_INT64_P = 19,
  TYPE_UINT64_P = 20,
  TYPE_FLOAT_P = 21,
  TYPE_DOUBLE_P = 22,
  TYPE_VOID_PP = 23,

  TYPE_VAR = 24,
  TYPE_ARRAY_BUFFER = 25,
  TYPE_ARRAY = 26,
  TYPE_DICTIONARY = 27,

  TYPE_FUNC_GET_VOID_P = 28,
  TYPE_FUNC_GET_INT8 = 29,
  TYPE_FUNC_GET_UINT8 = 30,
  TYPE_FUNC_GET_INT16 = 31,
  TYPE_FUNC_GET_UINT16 = 32,
  TYPE_FUNC_GET_INT32 = 33,
  TYPE_FUNC_GET_UINT32 = 34,
  TYPE_FUNC_GET_INT64 = 35,
  TYPE_FUNC_GET_UINT64 = 36,
  TYPE_FUNC_GET_FLOAT = 37,
  TYPE_FUNC_GET_DOUBLE = 38,

  TYPE_FUNC_SET_VOID_P = 39,
  TYPE_FUNC_SET_INT8 = 40,
  TYPE_FUNC_SET_UINT8 = 41,
  TYPE_FUNC_SET_INT16 = 42,
  TYPE_FUNC_SET_UINT16 = 43,
  TYPE_FUNC_SET_INT32 = 44,
  TYPE_FUNC_SET_UINT32 = 45,
  TYPE_FUNC_SET_INT64 = 46,
  TYPE_FUNC_SET_UINT64 = 47,
  TYPE_FUNC_SET_FLOAT = 48,
  TYPE_FUNC_SET_DOUBLE = 49,

  TYPE_FUNC_FREE = 50,
  TYPE_FUNC_MALLOC = 51,
  TYPE_FUNC_MEMSET = 52,
  TYPE_FUNC_MEMCPY = 53,

  TYPE_FUNC_ADDREF_RELEASE = 54,
  TYPE_FUNC_ARRAY_BUFFER_CREATE = 55,
  TYPE_FUNC_ARRAY_BUFFER_MAP = 56,
  TYPE_FUNC_ARRAY_BUFFER_UNMAP = 57,

  TYPE_FUNC_BINOP_VOID_P_INT32 = 58,
  TYPE_FUNC_BINOP_INT32 = 59,
  TYPE_FUNC_BINOP_UINT32 = 60,
  TYPE_FUNC_BINOP_INT64 = 61,
  TYPE_FUNC_BINOP_UINT64 = 62,
  TYPE_FUNC_BINOP_FLOAT = 63,
  TYPE_FUNC_BINOP_DOUBLE = 64,

  TYPE_Z_STREAM = 65,
  TYPE_Z_STREAM_P = 66,
  TYPE_FUNC_DEFLATE = 67,
  TYPE_FUNC_COMPRESS = 68,
  TYPE_FUNC_COMPRESS_BOUND = 69,
  NUM_TYPES
} Type;

const char* TypeToString(Type);

#endif  // TYPE_H_
