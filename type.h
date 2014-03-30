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

enum Type {
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
  TYPE_ARRAY_BUFFER = 17,
  TYPE_ARRAY = 18,
  TYPE_DICTIONARY = 19,
  TYPE_FUNC_MALLOC = 20,
  TYPE_FUNC_MEMSET = 21,
  TYPE_FUNC_MEMCPY = 22,
  TYPE_FUNC_ADD = 23,
  TYPE_FUNC_SET_UINT8_P = 24,
  TYPE_FUNC_SET_UINT32 = 25,
  TYPE_FUNC_GET_UINT8_P = 26,
  TYPE_FUNC_GET_UINT32 = 27,
  TYPE_FUNC_SUB_INT32 = 28,
  TYPE_FUNC_SUB_UINT32 = 29,
  TYPE_FUNC_ARRAY_BUFFER_CREATE = 30,
  TYPE_FUNC_ARRAY_BUFFER_MAP = 31,
  TYPE_FUNC_ARRAY_BUFFER_UNMAP = 32,
  TYPE_Z_STREAM = 33,
  TYPE_Z_STREAM_P = 34,
  TYPE_FUNC_DEFLATE = 35,
  NUM_TYPES
};

const char* TypeToString(Type);

#endif  // TYPE_H_
