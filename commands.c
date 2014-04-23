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

#include "commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "bool.h"
#include "error.h"
#include "interfaces.h"
#include "type.h"
#include "var.h"

static void Handle_destroyHandles(Command* command);

static void Handle_add(Command* command);
static void Handle_arrayBufferByteLength(Command* command);
static void Handle_arrayBufferCreate(Command* command);
static void Handle_arrayBufferMap(Command* command);
static void Handle_arrayBufferUnmap(Command* command);
static void Handle_arrayCreate(Command* command);
static void Handle_arrayGet(Command* command);
static void Handle_arrayGetLength(Command* command);
static void Handle_arraySet(Command* command);
static void Handle_arraySetLength(Command* command);
static void Handle_compressBound(Command* command);
static void Handle_compress(Command* command);
static void Handle_deflate(Command* command);
static void Handle_deflateInit(Command* command);
static void Handle_dictCreate(Command* command);
static void Handle_dictDelete(Command* command);
static void Handle_dictGet(Command* command);
static void Handle_dictHasKey(Command* command);
static void Handle_dictSet(Command* command);
static void Handle_free(Command* command);
static void Handle_get(Command* command);
static void Handle_malloc(Command* command);
static void Handle_memcpy(Command* command);
static void Handle_memset(Command* command);
static void Handle_set(Command* command);
static void Handle_strlen(Command* command);
static void Handle_sub(Command* command);
static void Handle_varAddRef(Command* command);
static void Handle_varFromUtf8(Command* command);
static void Handle_varRelease(Command* command);
static void Handle_varToUtf8(Command* command);
static void Handle_zlibVersion(Command* command);

typedef void (*HandleFunc)(Command*);
typedef struct {
  const char* name;
  HandleFunc func;
} NameFunc;

// TODO(binji): hashmap
static NameFunc g_FuncMap[] = {
  {"*destroyHandles", Handle_destroyHandles},
  {"add", Handle_add},
  {"arrayBufferByteLength", Handle_arrayBufferByteLength},
  {"arrayBufferCreate", Handle_arrayBufferCreate},
  {"arrayBufferMap", Handle_arrayBufferMap},
  {"arrayBufferUnmap", Handle_arrayBufferUnmap},
  {"arrayCreate", Handle_arrayCreate},
  {"arrayGet", Handle_arrayGet},
  {"arrayGetLength", Handle_arrayGetLength},
  {"arraySet", Handle_arraySet},
  {"arraySetLength", Handle_arraySetLength},
  {"compressBound", Handle_compressBound},
  {"compress", Handle_compress},
  {"deflate", Handle_deflate},
  {"deflateInit", Handle_deflateInit},
  {"dictCreate", Handle_dictCreate},
  {"dictDelete", Handle_dictDelete},
  {"dictGet", Handle_dictGet},
  {"dictHasKey", Handle_dictHasKey},
  {"dictSet", Handle_dictSet},
  {"free", Handle_free},
  {"get", Handle_get},
  {"malloc", Handle_malloc},
  {"memcpy", Handle_memcpy},
  {"memset", Handle_memset},
  {"set", Handle_set},
  {"strlen", Handle_strlen},
  {"sub", Handle_sub},
  {"varAddRef", Handle_varAddRef},
  {"varFromUtf8", Handle_varFromUtf8},
  {"varRelease", Handle_varRelease},
  {"varToUtf8", Handle_varToUtf8},
  {"zlibVersion", Handle_zlibVersion},
  {NULL, NULL},
};

void HandleCommand(Command* command) {
  NameFunc* name_func = &g_FuncMap[0];
  for (; name_func->name; name_func++) {
    if (strcmp(name_func->name, command->command) == 0) {
      name_func->func(command);
      return;
    }
  }

  VERROR("Unknown command: %s", command->command);
}

#define TYPE_CHECK(expected) \
  VERROR_IF(command->type == expected, \
            "Type mismatch. Expected %s. Got %s.", \
            TypeToString(expected), \
            TypeToString(command->type))

#define TYPE_FAIL \
  VERROR("Type didn't match any types. Got %s.", TypeToString(command->type))

#define CMD_VERROR(fmt, ...) \
  VERROR("%s: " fmt, command->command, __VA_ARGS__)

#define CMD_ERROR(msg) \
  VERROR("%s: " msg, command->command)

static bool GetArgVoidp(Command* command, int32_t index, void** out_value) {
  struct PP_Var arg_var;
  bool arg_handle;
  if (!GetCommandArg(command, index, &arg_var, &arg_handle)) {
    CMD_VERROR("Can't get arg %d", index);
    return FALSE;
  }
  if (!arg_handle) {
    CMD_VERROR("Expected arg %d to be handle", index);
    return FALSE;
  }
  int32_t arg_handle_int;
  if (!GetVarInt32(&arg_var, &arg_handle_int)) {
    CMD_VERROR("Expected handle arg %d to be int32_t", index);
    return FALSE;
  }

  Handle handle = arg_handle_int;
  if (!GetHandleVoidp(handle, out_value)) {
    CMD_VERROR("Expected arg %d handle's value to be void*", index);
    return FALSE;
  }

  return TRUE;
}

static bool GetArgInt32(Command* command, int32_t index, int32_t* out_value) {
  struct PP_Var arg_var;
  bool arg_handle;
  if (!GetCommandArg(command, index, &arg_var, &arg_handle)) {
    CMD_VERROR("Can't get arg %d", index);
    return FALSE;
  }
  if (arg_handle) {
    int32_t arg_handle_int;
    if (!GetVarInt32(&arg_var, &arg_handle_int)) {
      CMD_VERROR("Expected handle arg %d to be int32_t", index);
      return FALSE;
    }

    Handle handle = arg_handle_int;
    if (!GetHandleInt32(handle, out_value)) {
      CMD_VERROR("Expected arg %d handle's value to be int32_t", index);
      return FALSE;
    }
  } else {
    if (!GetVarInt32(&arg_var, out_value)) {
      CMD_VERROR("Expected arg %d to be int32_t", index);
      return FALSE;
    }
  }

  return TRUE;
}

static bool GetArgUint32(Command* command, int32_t index, uint32_t* out_value) {
  struct PP_Var arg_var;
  bool arg_handle;
  if (!GetCommandArg(command, index, &arg_var, &arg_handle)) {
    CMD_VERROR("Can't get arg %d", index);
    return FALSE;
  }
  if (arg_handle) {
    int32_t arg_handle_int;
    if (!GetVarInt32(&arg_var, &arg_handle_int)) {
      CMD_VERROR("Expected handle arg %d to be int32_t", index);
      return FALSE;
    }

    Handle handle = arg_handle_int;
    if (!GetHandleUint32(handle, out_value)) {
      CMD_VERROR("Expected arg %d handle's value to be uint32_t", index);
      return FALSE;
    }
  } else {
    if (!GetVarUint32(&arg_var, out_value)) {
      CMD_VERROR("Expected arg %d to be uint32_t", index);
      return FALSE;
    }
  }

  return TRUE;
}

static bool GetArgVar(Command* command, int32_t index,
                      struct PP_Var* out_value) {
  struct PP_Var arg_var;
  bool arg_handle;
  if (!GetCommandArg(command, index, &arg_var, &arg_handle)) {
    CMD_VERROR("Can't get arg %d", index);
    return FALSE;
  }
  if (arg_handle) {
    int32_t arg_handle_int;
    if (!GetVarInt32(&arg_var, &arg_handle_int)) {
      CMD_VERROR("Expected handle arg %d to be int32_t", index);
      return FALSE;
    }

    Handle handle = arg_handle_int;
    if (!GetHandleVar(handle, out_value)) {
      CMD_VERROR("Expected arg %d handle's value to be uint32_t", index);
      return FALSE;
    }
  } else {
    *out_value = arg_var;
  }

  return TRUE;
}

void Handle_destroyHandles(Command* command) {
  int32_t index;
  int32_t handle_count = GetCommandArgCount(command);
  // TODO(binji): use malloc if handle_count is large?
  Handle* handles = alloca(handle_count * sizeof(Handle));
  for (index = 0; index < handle_count; ++index) {
    struct PP_Var arg_var;
    bool arg_handle;
    if (!GetCommandArg(command, index, &arg_var, &arg_handle)) {
      CMD_VERROR("Can't get arg %d", index);
      return;
    }

    if (!arg_handle) {
      CMD_VERROR("Expected arg %d to be a Handle", index);
      return;
    }

    int32_t arg_handle_int;
    if (!GetVarInt32(&arg_var, &arg_handle_int)) {
      CMD_VERROR("Expected handle arg %d to be int32_t", index);
      return;
    }

    Handle handle = arg_handle_int;
    handles[index] = handle;
  }

  DestroyHandles(handles, handle_count);
  printf("destroyHandles()\n");
}

void Handle_add(Command* command) {
  TYPE_CHECK(TYPE_FUNC_BINOP_VOID_P_INT32);
  void* arg0;
  int32_t arg1;
  if (!GetArgVoidp(command, 0, &arg0)) return;
  if (!GetArgInt32(command, 1, &arg1)) return;
  void* result = ((uint8_t*)arg0) + arg1;
  RegisterHandleVoidp(command->ret_handle, result);

  printf("add(%p, %d) => %p (%d)\n", arg0, arg1, result, command->ret_handle);
}

void Handle_arrayBufferByteLength(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_BUFFER_BYTE_LENGTH);
  struct PP_Var arg0;
  if (!GetArgVar(command, 0, &arg0)) return;
  void* arg1_voidp;
  if (!GetArgVoidp(command, 1, &arg1_voidp)) return;
  uint32_t* arg1 = (uint32_t*)arg1_voidp;
  int32_t result = (int32_t)g_ppb_var_array_buffer->ByteLength(arg0, arg1);
  RegisterHandleInt32(command->ret_handle, result);
  printf("arrayBufferByteLength(%lld, %p) => %d (%d)\n", arg0.value.as_id, arg1,
         result, command->ret_handle);
}


void Handle_arrayBufferCreate(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_BUFFER_CREATE);
  uint32_t arg0;
  if (!GetArgUint32(command, 0, &arg0)) return;
  struct PP_Var result = g_ppb_var_array_buffer->Create(arg0);
  if (result.type != PP_VARTYPE_ARRAY_BUFFER) {
    CMD_VERROR("Couldn't create ArrayBuffer of size %u", arg0);
    return;
  }
  RegisterHandleVar(command->ret_handle, result);
  // NOTE: releasing Var here so it is owned by the handle.
  ReleaseVar(&result);
  printf("arrayBufferCreate(%u) => %lld (%d)\n", arg0, result.value.as_id,
         command->ret_handle);
}

void Handle_arrayBufferMap(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_BUFFER_MAP);
  struct PP_Var arg0;
  if (!GetArgVar(command, 0, &arg0)) return;
  void* result = g_ppb_var_array_buffer->Map(arg0);
  RegisterHandleVoidp(command->ret_handle, result);
  printf("arrayBufferMap(%lld) => %p (%d)\n", arg0.value.as_id,
         result, command->ret_handle);
}

void Handle_arrayBufferUnmap(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_BUFFER_UNMAP);
  struct PP_Var arg0;
  if (!GetArgVar(command, 0, &arg0)) return;
  g_ppb_var_array_buffer->Unmap(arg0);
  printf("arrayBufferUnmap(%lld)\n", arg0.value.as_id);
}

void Handle_arrayCreate(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_CREATE);
  struct PP_Var result = g_ppb_var_array->Create();
  if (result.type != PP_VARTYPE_ARRAY) {
    CMD_ERROR("Couldn't create Array.");
    return;
  }
  RegisterHandleVar(command->ret_handle, result);
  // NOTE: releasing Var here so it is owned by the handle.
  ReleaseVar(&result);
  printf("arrayCreate() => %lld (%d)\n", result.value.as_id,
         command->ret_handle);
}

void Handle_arrayGet(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_GET);
  struct PP_Var arg0;
  if (!GetArgVar(command, 0, &arg0)) return;
  uint32_t arg1;
  if (!GetArgUint32(command, 1, &arg1)) return;
  struct PP_Var result = g_ppb_var_array->Get(arg0, arg1);
  RegisterHandleVar(command->ret_handle, result);
  // TODO(binji): describe var
  printf("arrayGet(%lld, %u) => <Var> (%d)\n", arg0.value.as_id, arg1,
         command->ret_handle);
}

void Handle_arrayGetLength(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_GET_LENGTH);
  struct PP_Var arg0;
  if (!GetArgVar(command, 0, &arg0)) return;
  uint32_t result = g_ppb_var_array->GetLength(arg0);
  RegisterHandleUint32(command->ret_handle, result);
  printf("arrayGetLength(%lld) => %u (%d)\n", arg0.value.as_id, result,
         command->ret_handle);
}

void Handle_arraySet(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_SET);
  struct PP_Var arg0;
  if (!GetArgVar(command, 0, &arg0)) return;
  uint32_t arg1;
  if (!GetArgUint32(command, 1, &arg1)) return;
  struct PP_Var arg2;
  if (!GetArgVar(command, 2, &arg2)) return;
  int32_t result = (int32_t)g_ppb_var_array->Set(arg0, arg1, arg2);
  RegisterHandleInt32(command->ret_handle, result);
  // TODO(binji): describe var
  printf("arraySet(%lld, %u, <Var>) => %d (%d)\n", arg0.value.as_id, arg1,
         result, command->ret_handle);
}

void Handle_arraySetLength(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_SET_LENGTH);
  struct PP_Var arg0;
  if (!GetArgVar(command, 0, &arg0)) return;
  uint32_t arg1;
  if (!GetArgUint32(command, 1, &arg1)) return;
  int32_t result = (int32_t)g_ppb_var_array->SetLength(arg0, arg1);
  RegisterHandleUint32(command->ret_handle, result);
  printf("arraySetLength(%lld, %u) => %d (%d)\n", arg0.value.as_id, arg1,
         result, command->ret_handle);
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

void Handle_dictCreate(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DICT_CREATE);
  struct PP_Var result = g_ppb_var_dictionary->Create();
  RegisterHandleVar(command->ret_handle, result);
  printf("dictCreate() => %lld (%d)\n", result.value.as_id,
         command->ret_handle);
  if (result.type != PP_VARTYPE_DICTIONARY) {
    CMD_ERROR("Couldn't create Dictionary.");
    return;
  }
  RegisterHandleVar(command->ret_handle, result);
  // NOTE: releasing Var here so it is owned by the handle.
  ReleaseVar(&result);
  printf("dictCreate() => %lld (%d)\n", result.value.as_id,
         command->ret_handle);
}

void Handle_dictDelete(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DICT_DELETE);
  struct PP_Var arg0;
  if (!GetArgVar(command, 0, &arg0)) return;
  struct PP_Var arg1;
  if (!GetArgVar(command, 1, &arg1)) return;
  g_ppb_var_dictionary->Delete(arg0, arg1);
  printf("dictDelete(%lld, %lld)\n", arg0.value.as_id, arg1.value.as_id);
}

void Handle_dictGet(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DICT_GET);
  struct PP_Var arg0;
  if (!GetArgVar(command, 0, &arg0)) return;
  struct PP_Var arg1;
  if (!GetArgVar(command, 1, &arg1)) return;
  struct PP_Var result = g_ppb_var_dictionary->Get(arg0, arg1);
  RegisterHandleVar(command->ret_handle, result);
  printf("dictGet(%lld, %lld) => <Var> (%d)\n", arg0.value.as_id,
         arg1.value.as_id, command->ret_handle);
}

void Handle_dictHasKey(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DICT_HAS_KEY);
  struct PP_Var arg0;
  if (!GetArgVar(command, 0, &arg0)) return;
  struct PP_Var arg1;
  if (!GetArgVar(command, 1, &arg1)) return;
  int32_t result = (int32_t)g_ppb_var_dictionary->HasKey(arg0, arg1);
  RegisterHandleInt32(command->ret_handle, result);
  printf("dictHasKey(%lld, %lld) => %d (%d)\n", arg0.value.as_id,
         arg1.value.as_id, result, command->ret_handle);
}

void Handle_dictSet(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DICT_SET);
  struct PP_Var arg0;
  if (!GetArgVar(command, 0, &arg0)) return;
  struct PP_Var arg1;
  if (!GetArgVar(command, 1, &arg1)) return;
  struct PP_Var arg2;
  if (!GetArgVar(command, 2, &arg2)) return;
  int32_t result = (int32_t)g_ppb_var_dictionary->Set(arg0, arg1, arg2);
  RegisterHandleInt32(command->ret_handle, result);
  printf("dictSet(%lld, %lld, <Var>) => %d (%d)\n", arg0.value.as_id,
         arg1.value.as_id, result, command->ret_handle);
}

void Handle_free(Command* command) {
  TYPE_CHECK(TYPE_FUNC_FREE);
  void* arg0;
  if (!GetArgVoidp(command, 0, &arg0)) return;
  free(arg0);
  printf("free(%p)\n", arg0);
}

void Handle_get(Command* command) {
  switch (command->type) {
    case TYPE_FUNC_GET_VOID_P: {
      void* arg0_voidp;
      if (!GetArgVoidp(command, 0, &arg0_voidp)) return;
      void** arg0 = (void**)arg0_voidp;
      void* result = *arg0;
      HandleValue hval;
      hval.voidp = result;
      RegisterHandle(command->ret_handle, TYPE_VOID_P, hval);
      printf("*(void**)%p => %p\n", arg0, result);
      break;
    }
    case TYPE_FUNC_GET_UINT32: {
      void* arg0_voidp;
      if (!GetArgVoidp(command, 0, &arg0_voidp)) return;
      uint32_t* arg0 = (uint32_t*)arg0_voidp;
      uint32_t result = *arg0;
      HandleValue hval;
      hval.uint32 = result;
      RegisterHandle(command->ret_handle, TYPE_UINT32, hval);
      printf("*(uint32_t*)%p => %u\n", arg0, result);
      break;
    }
    default:
      TYPE_FAIL;
      break;
  }
}

void Handle_malloc(Command* command) {
  TYPE_CHECK(TYPE_FUNC_MALLOC);
  uint32_t arg0;
  if (!GetArgUint32(command, 0, &arg0)) return;
  void* result = malloc(arg0);
  RegisterHandleVoidp(command->ret_handle, result);
  printf("malloc(%u) => %p (%d)\n", arg0, result, command->ret_handle);
}

void Handle_memcpy(Command* command) {
  TYPE_CHECK(TYPE_FUNC_MEMCPY);
  void* dst;
  void* src;
  size_t size;
  if (!GetArgVoidp(command, 0, &dst)) return;
  if (!GetArgVoidp(command, 1, &src)) return;
  if (!GetArgUint32(command, 2, &size)) return;
  memcpy(dst, src, size);
  printf("memcpy(%p, %p, %u)\n", dst, src, size);
}

void Handle_memset(Command* command) {
  TYPE_CHECK(TYPE_FUNC_MEMSET);
  void* buffer;
  int32_t value;
  size_t size;
  if (!GetArgVoidp(command, 0, &buffer)) return;
  if (!GetArgInt32(command, 1, &value)) return;
  if (!GetArgUint32(command, 2, &size)) return;
  memset(buffer, value, size);
  printf("memset(%p, %d, %u)\n", buffer, value, size);
}

void Handle_set(Command* command) {
  switch (command->type) {
    case TYPE_FUNC_SET_VOID_P: {
      void* arg0_voidp;
      if (!GetArgVoidp(command, 0, &arg0_voidp)) return;
      void** arg0 = (void**)arg0_voidp;
      void* arg1_voidp;
      if (!GetArgVoidp(command, 1, &arg1_voidp)) return;
      void* arg1 = (void*)arg1_voidp;
      *arg0 = arg1;
      printf("*(void**)%p = %p\n", arg0, arg1);
      break;
    }
    case TYPE_FUNC_SET_UINT32: {
      void* arg0_voidp;
      if (!GetArgVoidp(command, 0, &arg0_voidp)) return;
      uint32_t* arg0 = (uint32_t*)arg0_voidp;
      uint32_t arg1;
      if (!GetArgUint32(command, 1, &arg1)) return;
      *arg0 = arg1;
      printf("*(uint32_t*)%p = %u\n", arg0, arg1);
      break;
    }
    default:
      TYPE_FAIL;
      break;
  }
}

void Handle_strlen(Command* command) {
  TYPE_CHECK(TYPE_FUNC_STRLEN);
  void* arg0_voidp;
  if (!GetArgVoidp(command, 0, &arg0_voidp)) return;
  const char* arg0 = (const char*)arg0_voidp;
  uint32_t result = (uint32_t)strlen(arg0);
  RegisterHandleUint32(command->ret_handle, result);
  printf("strlen(\"%s\") => %u (%d)\n", arg0, result, command->ret_handle);
}

void Handle_sub(Command* command) {
  switch (command->type) {
    case TYPE_FUNC_BINOP_INT32: {
      int32_t arg0;
      int32_t arg1;
      if (!GetArgInt32(command, 0, &arg0)) return;
      if (!GetArgInt32(command, 1, &arg1)) return;
      int32_t result = arg0 - arg1;
      RegisterHandleInt32(command->ret_handle, result);
      printf("sub(%d, %d) => %d (%d)\n", arg0, arg1, result,
             command->ret_handle);
      break;
    }
    case TYPE_FUNC_BINOP_UINT32: {
      uint32_t arg0;
      uint32_t arg1;
      if (!GetArgUint32(command, 0, &arg0)) return;
      if (!GetArgUint32(command, 1, &arg1)) return;
      uint32_t result = arg0 - arg1;
      RegisterHandleUint32(command->ret_handle, result);
      printf("sub(%u, %u) => %u (%d)\n", arg0, arg1, result,
             command->ret_handle);
      break;
    }
    default:
      TYPE_FAIL;
      break;
  }
}

void Handle_varAddRef(Command* command) {
  TYPE_CHECK(TYPE_FUNC_VAR_ADDREF_RELEASE);
  struct PP_Var arg0;
  if (!GetArgVar(command, 0, &arg0)) return;
  g_ppb_var->AddRef(arg0);
  printf("varAddRef(%lld)\n", arg0.value.as_id);
}

void Handle_varFromUtf8(Command* command) {
  TYPE_CHECK(TYPE_FUNC_VAR_FROM_UTF8);
  void* arg0_voidp;
  if (!GetArgVoidp(command, 0, &arg0_voidp)) return;
  const char* arg0 = (const char*)arg0_voidp;
  uint32_t arg1;
  if (!GetArgUint32(command, 1, &arg1)) return;
  struct PP_Var result = g_ppb_var->VarFromUtf8(arg0, arg1);
  RegisterHandleVar(command->ret_handle, result);
  printf("varFromUtf8(%p, %u) => %lld (%d)\n", arg0, arg1, result.value.as_id,
         command->ret_handle);
}

void Handle_varRelease(Command* command) {
  TYPE_CHECK(TYPE_FUNC_VAR_ADDREF_RELEASE);
  struct PP_Var arg0;
  if (!GetArgVar(command, 0, &arg0)) return;
  g_ppb_var->Release(arg0);
  printf("varRelease(%lld)\n", arg0.value.as_id);
}

void Handle_varToUtf8(Command* command) {
  TYPE_CHECK(TYPE_FUNC_VAR_TO_UTF8);
  struct PP_Var arg0;
  if (!GetArgVar(command, 0, &arg0)) return;
  void* arg1_voidp;
  if (!GetArgVoidp(command, 1, &arg1_voidp)) return;
  uint32_t* arg1 = (uint32_t*)arg1_voidp;
  void* result = (void*)g_ppb_var->VarToUtf8(arg0, arg1);
  RegisterHandleVoidp(command->ret_handle, result);
  printf("varToUtf8(%lld, %p) => %p (%d)\n", arg0.value.as_id, arg1, result,
         command->ret_handle);
}

void Handle_zlibVersion(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ZLIB_VERSION);
  void* result = (void*)zlibVersion();
  RegisterHandleVoidp(command->ret_handle, result);
  printf("zlibVersion() => \"%s\" (%d)\n", (const char*)result,
         command->ret_handle);
}
