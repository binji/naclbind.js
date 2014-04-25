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

#include "zip_commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <zlib.h>

#include "zip.h"

#include "bool.h"
#include "commands.h"
#include "error.h"
#include "interfaces.h"
#include "type.h"
#include "var.h"
#include "zip_type.h"

static void Handle_zipOpen(Command* command);
static void Handle_zipOpenNewFileInZip(Command* command);
static void Handle_zipWriteInFileInZip(Command* command);
static void Handle_zipCloseFileInZip(Command* command);
static void Handle_zipClose(Command* command);
static void Handle_fopen(Command* command);
static void Handle_fread(Command* command);
static void Handle_fclose(Command* command);
static void Handle_stat(Command* command);

typedef void (*HandleFunc)(Command*);
typedef struct {
  const char* name;
  HandleFunc func;
} NameFunc;

// TODO(binji): hashmap
static NameFunc g_FuncMap[] = {
  {"zipOpen", Handle_zipOpen},
  {"zipOpenNewFileInZip", Handle_zipOpenNewFileInZip},
  {"zipWriteInFileInZip", Handle_zipWriteInFileInZip},
  {"zipCloseFileInZip", Handle_zipCloseFileInZip},
  {"zipClose", Handle_zipClose},
  {"fopen", Handle_fopen},
  {"fread", Handle_fread},
  {"fclose", Handle_fclose},
  {"stat", Handle_stat},
  {NULL, NULL},
};

bool HandleZipCommand(Command* command) {
  NameFunc* name_func = &g_FuncMap[0];
  for (; name_func->name; name_func++) {
    if (strcmp(name_func->name, command->command) == 0) {
      name_func->func(command);
      return TRUE;
    }
  }
  return FALSE;
}

void Handle_zipOpen(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ZIP_OPEN);
  ARG_VOIDP_CAST(0, const char*);
  ARG_INT(1);
  zipFile result = zipOpen(arg0, arg1);
  RegisterHandleVoidp(command->ret_handle, result);
  printf("zipOpen(\"%s\", %d) => %p (%d)\n", arg0, arg1, result,
         command->ret_handle);
}

void Handle_zipOpenNewFileInZip(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ZIP_OPEN_NEW_FILE_IN_ZIP);
  ARG_VOIDP_CAST(0, zipFile);
  ARG_VOIDP_CAST(1, const char*);
  ARG_VOIDP_CAST(2, zip_fileinfo*);
  ARG_VOIDP(3);
  ARG_UINT(4);
  ARG_VOIDP(5);
  ARG_UINT(6);
  ARG_VOIDP_CAST(7, const char*);
  ARG_INT(8);
  ARG_INT(9);
  int32_t result = (int32_t)zipOpenNewFileInZip(arg0, arg1, arg2, arg3, arg4,
                                                arg5, arg6, arg7, arg8, arg9);
  RegisterHandleInt32(command->ret_handle, result);
  printf("zipOpenInNewFileInZip(%p, \"%s\", %p, %p, %u, %p, %u, \"%s\", %d, %d) => %d (%d)\n",
         arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9,
         result, command->ret_handle);
}

void Handle_zipWriteInFileInZip(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ZIP_WRITE_IN_FILE_IN_ZIP);
  ARG_VOIDP_CAST(0, zipFile);
  ARG_VOIDP(1);
  ARG_UINT(2);
  int32_t result = (int32_t)zipWriteInFileInZip(arg0, arg1, arg2);
  RegisterHandleInt32(command->ret_handle, result);
  printf("zipWriteInFileInZip(%p, %p, %u) => %d (%d)\n", arg0, arg1, arg2,
         result, command->ret_handle);
}

void Handle_zipCloseFileInZip(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ZIP_CLOSE_FILE_IN_ZIP);
  ARG_VOIDP_CAST(0, zipFile);
  int32_t result = (int32_t)zipCloseFileInZip(arg0);
  RegisterHandleInt32(command->ret_handle, result);
  printf("zipCloseFileInZip(%p) => %d (%d)\n", arg0,
         result, command->ret_handle);
}

void Handle_zipClose(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ZIP_CLOSE);
  ARG_VOIDP_CAST(0, zipFile);
  ARG_VOIDP_CAST(1, const char*);
  int32_t result = (int32_t)zipClose(arg0, arg1);
  RegisterHandleInt32(command->ret_handle, result);
  printf("zipClose(%p, \"%s\") => %d (%d)\n", arg0, arg1,
         result, command->ret_handle);
}

void Handle_fopen(Command* command) {
  TYPE_CHECK(TYPE_FUNC_FOPEN);
  ARG_VOIDP_CAST(0, const char*);
  ARG_VOIDP_CAST(1, const char*);
  void* result = (void*)fopen(arg0, arg1);
  RegisterHandleVoidp(command->ret_handle, result);
  printf("fopen(\"%s\", \"%s\") => %p (%d)\n", arg0, arg1, result,
         command->ret_handle);
}

void Handle_fread(Command* command) {
  TYPE_CHECK(TYPE_FUNC_FREAD);
  ARG_VOIDP(0);
  ARG_UINT(1);
  ARG_UINT(2);
  ARG_VOIDP_CAST(3, FILE*);
  int32_t result = (int32_t)fread(arg0, arg1, arg2, arg3);
  RegisterHandleInt32(command->ret_handle, result);
  printf("fread(%p, %u, %u, %p) => %d (%d)\n", arg0, arg1, arg2, arg3, result,
         command->ret_handle);
}

void Handle_fclose(Command* command) {
  TYPE_CHECK(TYPE_FUNC_FCLOSE);
  ARG_VOIDP_CAST(0, FILE*);
  int32_t result = (int32_t)fclose(arg0);
  RegisterHandleInt32(command->ret_handle, result);
  printf("fclose(%p) => %d (%d)\n", arg0, result, command->ret_handle);
}

void Handle_stat(Command* command) {
  TYPE_CHECK(TYPE_FUNC_STAT);
  ARG_VOIDP_CAST(0, const char*);
  ARG_VOIDP_CAST(1, struct stat*);
  int32_t result = (int32_t)stat(arg0, arg1);
  RegisterHandleInt32(command->ret_handle, result);
  printf("stat(\"%s\", %p) => %d (%d)\n", arg0, arg1, result,
         command->ret_handle);
}
