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

#include "zip_type.h"

static const char* kTypeString[] = {
  /* 80 */ "tm_zip_s",
  /* 81 */ "zip_fileinfo",
  /* 82 */ "zip_fileinfo*",
  /* 83 */ "zipFile",
  /* 84 */ "zipFile*",
  /* 85 */ "zipFile (*)(const char*, int)",
  /* 86 */ "int (*)(zipFile, const char*, const zip_fileinfo*, const void*, uInt, const void*, uInt, const char*, int, int)",
  /* 87 */ "int (*)(zipFile, const void*, unsigned)",
  /* 88 */ "int (*)(zipFile)",
  /* 89 */ "int (*)(zipFile, const char*)",

  /* 90 */ "FILE",
  /* 91 */ "FILE*",
  /* 92 */ "struct stat",
  /* 93 */ "struct stat*",
  /* 94 */ "FILE* (*)(const char*, const char*)",
  /* 95 */ "int (*)(void*, size_t, size_t, FILE*)",
  /* 96 */ "int (*)(FILE*)",
  /* 97 */ "int (*)(const char*, struct stat*)",
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

