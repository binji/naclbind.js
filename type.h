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
  TYPE_SIZE_T = 12,
  TYPE_VOID_P = 13,
  TYPE_UINT8_P = 14,
  TYPE_UINT8_PP = 15,
  TYPE_UINT32_P = 16,
  TYPE_VAR = 17,
  TYPE_ARRAY_BUFFER = 18,
  TYPE_ARRAY = 19,
  TYPE_DICTIONARY = 20,
  TYPE_FUNC_ADDREF_RELEASE = 21,
  TYPE_FUNC_FREE = 22,
  TYPE_FUNC_MALLOC = 23,
  TYPE_FUNC_MEMSET = 24,
  TYPE_FUNC_MEMCPY = 25,
  TYPE_FUNC_ADD = 26,
  TYPE_FUNC_SET_UINT8_P = 27,
  TYPE_FUNC_SET_UINT32 = 28,
  TYPE_FUNC_GET_UINT8_P = 29,
  TYPE_FUNC_GET_UINT32 = 30,
  TYPE_FUNC_SUB_INT32 = 31,
  TYPE_FUNC_SUB_UINT32 = 32,
  TYPE_FUNC_ARRAY_BUFFER_CREATE = 33,
  TYPE_FUNC_ARRAY_BUFFER_MAP = 34,
  TYPE_FUNC_ARRAY_BUFFER_UNMAP = 35,
  TYPE_Z_STREAM = 36,
  TYPE_Z_STREAM_P = 37,
  TYPE_FUNC_DEFLATE = 38,
  TYPE_FUNC_COMPRESS = 39,
  TYPE_FUNC_COMPRESS_BOUND = 40,
  NUM_TYPES
} Type;

const char* TypeToString(Type);

#endif  // TYPE_H_
