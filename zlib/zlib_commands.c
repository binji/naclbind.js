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

#include "zlib_commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "bool.h"
#include "commands.h"
#include "error.h"
#include "interfaces.h"
#include "type.h"
#include "var.h"
#include "zlib_type.h"

static void Handle_compressBound(Command* command);
static void Handle_compress(Command* command);
static void Handle_deflate(Command* command);
static void Handle_deflateInit(Command* command);
static void Handle_zlibVersion(Command* command);

typedef void (*HandleFunc)(Command*);
typedef struct {
  const char* name;
  HandleFunc func;
} NameFunc;

// TODO(binji): hashmap
static NameFunc g_FuncMap[] = {
  {"compressBound", Handle_compressBound},
  {"compress", Handle_compress},
  {"deflate", Handle_deflate},
  {"deflateInit", Handle_deflateInit},
  {"zlibVersion", Handle_zlibVersion},
  {NULL, NULL},
};

bool HandleZlibCommand(Command* command) {
  NameFunc* name_func = &g_FuncMap[0];
  for (; name_func->name; name_func++) {
    if (strcmp(name_func->name, command->command) == 0) {
      name_func->func(command);
      return TRUE;
    }
  }
  return FALSE;
}

void Handle_compress(Command* command) {
  TYPE_CHECK(TYPE_FUNC_COMPRESS);
  ARG_VOIDP_CAST(0, Bytef*);
  ARG_VOIDP_CAST(1, uLongf*);
  ARG_VOIDP_CAST(2, Bytef*);
  ARG_UINT_CAST(3, uLong);
  int result = compress(arg0, arg1, arg2, arg3);
  RegisterHandleInt32(command->ret_handle, result);
  printf("compress(%p, %p, %p, %lu) => %d\n", arg0, arg1, arg2, arg3, result);
}

void Handle_compressBound(Command* command) {
  TYPE_CHECK(TYPE_FUNC_COMPRESS_BOUND);
  ARG_UINT_CAST(0, uLong);
  uLong result = compressBound(arg0);
  RegisterHandleInt32(command->ret_handle, result);
  printf("compressBound(%lu) => %lu\n", arg0, result);
}

void Handle_deflate(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DEFLATE);
  ARG_VOIDP_CAST(0, z_stream*);
  ARG_INT(1);
  int result = deflate(arg0, arg1);
  RegisterHandleInt32(command->ret_handle, result);
  printf("deflate(%p, %d) => %d\n", arg0, arg1, result);
}

void Handle_deflateInit(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DEFLATE);
  ARG_VOIDP_CAST(0, z_stream*);
  ARG_INT(1);
  int result = deflateInit(arg0, arg1);
  RegisterHandleInt32(command->ret_handle, result);
  printf("deflateInit(%p, %d) => %d\n", arg0, arg1, result);
}

void Handle_zlibVersion(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ZLIB_VERSION);
  void* result = (void*)zlibVersion();
  RegisterHandleVoidp(command->ret_handle, result);
  printf("zlibVersion() => \"%s\" (%d)\n", (const char*)result,
         command->ret_handle);
}
