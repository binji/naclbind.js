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
  void* arg0_voidp;
  if (!GetArgVoidp(command, 0, &arg0_voidp)) return;
  Bytef* arg0 = (Bytef*)arg0_voidp;
  void* arg1_voidp;
  if (!GetArgVoidp(command, 1, &arg1_voidp)) return;
  uLongf* arg1 = (uLongf*)arg1_voidp;
  void* arg2_voidp;
  if (!GetArgVoidp(command, 2, &arg2_voidp)) return;
  Bytef* arg2 = (Bytef*)arg2_voidp;
  int arg3_int;
  if (!GetArgInt32(command, 3, &arg3_int)) return;
  uLong arg3 = (uLong)arg3_int;
  int result = compress(arg0, arg1, arg2, arg3);
  RegisterHandleInt32(command->ret_handle, result);
  printf("compress(%p, %p, %p, %lu) => %d\n", arg0, arg1, arg2, arg3, result);
}

void Handle_compressBound(Command* command) {
  TYPE_CHECK(TYPE_FUNC_COMPRESS_BOUND);
  int arg0_int;
  if (!GetArgInt32(command, 0, &arg0_int)) return;
  uLong arg0 = (uLong)arg0_int;
  uLong result = compressBound(arg0);
  RegisterHandleInt32(command->ret_handle, result);
  printf("compressBound(%lu) => %lu\n", arg0, result);
}

void Handle_deflate(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DEFLATE);
  void* arg0_voidp;
  if (!GetArgVoidp(command, 0, &arg0_voidp)) return;
  z_stream* arg0 = (z_stream*)arg0_voidp;
  int arg1;
  if (!GetArgInt32(command, 1, &arg1)) return;
  int result = deflate(arg0, arg1);
  RegisterHandleInt32(command->ret_handle, result);
  printf("deflate(%p, %d) => %d\n", arg0, arg1, result);
}

void Handle_deflateInit(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DEFLATE);
  void* arg0_voidp;
  if (!GetArgVoidp(command, 0, &arg0_voidp)) return;
  z_stream* arg0 = (z_stream*)arg0_voidp;
  int arg1;
  if (!GetArgInt32(command, 1, &arg1)) return;
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
