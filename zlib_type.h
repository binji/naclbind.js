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

#ifndef ZLIB_TYPE_H_
#define ZLIB_TYPE_H_

#include "type.h"

enum {
  TYPE_Z_STREAM = NUM_BUILTIN_TYPES,
  TYPE_Z_STREAM_P,
  TYPE_FUNC_DEFLATE,
  TYPE_FUNC_COMPRESS,
  TYPE_FUNC_COMPRESS_BOUND,
  TYPE_FUNC_ZLIB_VERSION,

  NUM_TYPES
};

const char* TypeToString(Type id);

#endif  // ZLIB_TYPE_H_
