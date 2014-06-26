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
static void Handle_puts(Command* command);
static void Handle_set(Command* command);
static void Handle_strlen(Command* command);
static void Handle_sub(Command* command);
static void Handle_varAddRef(Command* command);
static void Handle_varFromUtf8(Command* command);
static void Handle_varRelease(Command* command);
static void Handle_varToUtf8(Command* command);

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
  {"puts", Handle_puts},
  {"set", Handle_set},
  {"strlen", Handle_strlen},
  {"sub", Handle_sub},
  {"varAddRef", Handle_varAddRef},
  {"varFromUtf8", Handle_varFromUtf8},
  {"varRelease", Handle_varRelease},
  {"varToUtf8", Handle_varToUtf8},
  {NULL, NULL},
};

bool HandleBuiltinCommand(Command* command) {
  NameFunc* name_func = &g_FuncMap[0];
  for (; name_func->name; name_func++) {
    if (strcmp(name_func->name, command->command) == 0) {
      name_func->func(command);
      return TRUE;
    }
  }
  return FALSE;
}

bool GetArgVoidp(Command* command, int32_t index, void** out_value) {
  struct PP_Var arg_var;
  bool arg_handle;
  if (!GetCommandArg(command, index, &arg_var, &arg_handle)) {
    CMD_VERROR("Can't get arg %d", index);
    return FALSE;
  }
  if (arg_var.type == PP_VARTYPE_NULL) {
    *out_value = NULL;
    return TRUE;
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

bool GetArgInt32(Command* command, int32_t index, int32_t* out_value) {
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

bool GetArgUint32(Command* command, int32_t index, uint32_t* out_value) {
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

bool GetArgVar(Command* command, int32_t index, struct PP_Var* out_value) {
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
  ARG_VOIDP(0);
  ARG_INT(1);
  void* result = ((uint8_t*)arg0) + arg1;
  RegisterHandleVoidp(command->ret_handle, result);

  printf("add(%p, %d) => %p (%d)\n", arg0, arg1, result, command->ret_handle);
}

void Handle_arrayBufferByteLength(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_BUFFER_BYTE_LENGTH);
  ARG_VAR(0);
  ARG_VOIDP_CAST(1, uint32_t*);
  int32_t result = (int32_t)g_ppb_var_array_buffer->ByteLength(arg0, arg1);
  RegisterHandleInt32(command->ret_handle, result);
  printf("arrayBufferByteLength(%lld, %p) => %d (%d)\n", arg0.value.as_id, arg1,
         result, command->ret_handle);
}


void Handle_arrayBufferCreate(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_BUFFER_CREATE);
  ARG_UINT(0);
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
  ARG_VAR(0);
  void* result = g_ppb_var_array_buffer->Map(arg0);
  RegisterHandleVoidp(command->ret_handle, result);
  printf("arrayBufferMap(%lld) => %p (%d)\n", arg0.value.as_id,
         result, command->ret_handle);
}

void Handle_arrayBufferUnmap(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_BUFFER_UNMAP);
  ARG_VAR(0);
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
  ARG_VAR(0);
  ARG_UINT(1);
  struct PP_Var result = g_ppb_var_array->Get(arg0, arg1);
  RegisterHandleVar(command->ret_handle, result);
  // TODO(binji): describe var
  printf("arrayGet(%lld, %u) => <Var> (%d)\n", arg0.value.as_id, arg1,
         command->ret_handle);
}

void Handle_arrayGetLength(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_GET_LENGTH);
  ARG_VAR(0);
  uint32_t result = g_ppb_var_array->GetLength(arg0);
  RegisterHandleUint32(command->ret_handle, result);
  printf("arrayGetLength(%lld) => %u (%d)\n", arg0.value.as_id, result,
         command->ret_handle);
}

void Handle_arraySet(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_SET);
  ARG_VAR(0);
  ARG_UINT(1);
  ARG_VAR(2);
  int32_t result = (int32_t)g_ppb_var_array->Set(arg0, arg1, arg2);
  RegisterHandleInt32(command->ret_handle, result);
  // TODO(binji): describe var
  printf("arraySet(%lld, %u, <Var>) => %d (%d)\n", arg0.value.as_id, arg1,
         result, command->ret_handle);
}

void Handle_arraySetLength(Command* command) {
  TYPE_CHECK(TYPE_FUNC_ARRAY_SET_LENGTH);
  ARG_VAR(0);
  ARG_UINT(1);
  int32_t result = (int32_t)g_ppb_var_array->SetLength(arg0, arg1);
  RegisterHandleUint32(command->ret_handle, result);
  printf("arraySetLength(%lld, %u) => %d (%d)\n", arg0.value.as_id, arg1,
         result, command->ret_handle);
}

void Handle_dictCreate(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DICT_CREATE);
  struct PP_Var result = g_ppb_var_dictionary->Create();
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
  ARG_VAR(0);
  ARG_VAR(1);
  g_ppb_var_dictionary->Delete(arg0, arg1);
  printf("dictDelete(%lld, %lld)\n", arg0.value.as_id, arg1.value.as_id);
}

void Handle_dictGet(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DICT_GET);
  ARG_VAR(0);
  ARG_VAR(1);
  struct PP_Var result = g_ppb_var_dictionary->Get(arg0, arg1);
  RegisterHandleVar(command->ret_handle, result);
  printf("dictGet(%lld, %lld) => <Var> (%d)\n", arg0.value.as_id,
         arg1.value.as_id, command->ret_handle);
}

void Handle_dictHasKey(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DICT_HAS_KEY);
  ARG_VAR(0);
  ARG_VAR(1);
  int32_t result = (int32_t)g_ppb_var_dictionary->HasKey(arg0, arg1);
  RegisterHandleInt32(command->ret_handle, result);
  printf("dictHasKey(%lld, %lld) => %d (%d)\n", arg0.value.as_id,
         arg1.value.as_id, result, command->ret_handle);
}

void Handle_dictSet(Command* command) {
  TYPE_CHECK(TYPE_FUNC_DICT_SET);
  ARG_VAR(0);
  ARG_VAR(1);
  ARG_VAR(2);
  int32_t result = (int32_t)g_ppb_var_dictionary->Set(arg0, arg1, arg2);
  RegisterHandleInt32(command->ret_handle, result);
  printf("dictSet(%lld, %lld, <Var>) => %d (%d)\n", arg0.value.as_id,
         arg1.value.as_id, result, command->ret_handle);
}

void Handle_free(Command* command) {
  TYPE_CHECK(TYPE_FUNC_FREE);
  ARG_VOIDP(0);
  free(arg0);
  printf("free(%p)\n", arg0);
}

void Handle_get(Command* command) {
  switch (command->type) {
    case TYPE_FUNC_GET_VOID_P: {
      ARG_VOIDP_CAST(0, void**);
      void* result = *arg0;
      HandleValue hval;
      hval.voidp = result;
      RegisterHandle(command->ret_handle, TYPE_VOID_P, hval);
      printf("*(void**)%p => %p\n", arg0, result);
      break;
    }
    case TYPE_FUNC_GET_INT32: {
      ARG_VOIDP_CAST(0, int32_t*);
      int32_t result = *arg0;
      HandleValue hval;
      hval.int32 = result;
      RegisterHandle(command->ret_handle, TYPE_INT32, hval);
      printf("*(int32_t*)%p => %u\n", arg0, result);
      break;
    }
    case TYPE_FUNC_GET_UINT32: {
      ARG_VOIDP_CAST(0, uint32_t*);
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
  ARG_UINT(0);
  void* result = malloc(arg0);
  RegisterHandleVoidp(command->ret_handle, result);
  printf("malloc(%u) => %p (%d)\n", arg0, result, command->ret_handle);
}

void Handle_memcpy(Command* command) {
  TYPE_CHECK(TYPE_FUNC_MEMCPY);
  ARG_VOIDP(0);
  ARG_VOIDP(1);
  ARG_UINT(2);
  memcpy(arg0, arg1, arg2);
  printf("memcpy(%p, %p, %u)\n", arg0, arg1, arg2);
}

void Handle_memset(Command* command) {
  TYPE_CHECK(TYPE_FUNC_MEMSET);
  ARG_VOIDP(0);
  ARG_INT(1);
  ARG_UINT(2);
  memset(arg0, arg1, arg2);
  printf("memset(%p, %d, %u)\n", arg0, arg1, arg2);
}

void Handle_puts(Command* command) {
  TYPE_CHECK(TYPE_FUNC_PUTS);
  ARG_VOIDP_CAST(0, char*);
  int result = puts(arg0);
  RegisterHandleInt32(command->ret_handle, result);
  printf("puts(%p)\n", arg0);
}

void Handle_set(Command* command) {
  switch (command->type) {
    case TYPE_FUNC_SET_VOID_P: {
      ARG_VOIDP_CAST(0, void**);
      ARG_VOIDP(1);
      *arg0 = arg1;
      printf("*(void**)%p = %p\n", arg0, arg1);
      break;
    }
    case TYPE_FUNC_SET_INT32: {
      ARG_VOIDP_CAST(0, int32_t*);
      ARG_INT(1);
      *arg0 = arg1;
      printf("*(int32_t*)%p = %u\n", arg0, arg1);
      break;
    }
    case TYPE_FUNC_SET_UINT32: {
      ARG_VOIDP_CAST(0, uint32_t*);
      ARG_UINT(1);
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
  ARG_VOIDP_CAST(0, const char*);
  uint32_t result = (uint32_t)strlen(arg0);
  RegisterHandleUint32(command->ret_handle, result);
  printf("strlen(\"%s\") => %u (%d)\n", arg0, result, command->ret_handle);
}

void Handle_sub(Command* command) {
  switch (command->type) {
    case TYPE_FUNC_BINOP_INT32: {
      ARG_INT(0);
      ARG_INT(1);
      int32_t result = arg0 - arg1;
      RegisterHandleInt32(command->ret_handle, result);
      printf("sub(%d, %d) => %d (%d)\n", arg0, arg1, result,
             command->ret_handle);
      break;
    }
    case TYPE_FUNC_BINOP_UINT32: {
      ARG_UINT(0);
      ARG_UINT(1);
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
  ARG_VAR(0);
  g_ppb_var->AddRef(arg0);
  printf("varAddRef(%lld)\n", arg0.value.as_id);
}

void Handle_varFromUtf8(Command* command) {
  TYPE_CHECK(TYPE_FUNC_VAR_FROM_UTF8);
  ARG_VOIDP_CAST(0, const char*);
  ARG_UINT(1);
  struct PP_Var result = g_ppb_var->VarFromUtf8(arg0, arg1);
  RegisterHandleVar(command->ret_handle, result);
  printf("varFromUtf8(%p, %u) => %lld (%d)\n", arg0, arg1, result.value.as_id,
         command->ret_handle);
}

void Handle_varRelease(Command* command) {
  TYPE_CHECK(TYPE_FUNC_VAR_ADDREF_RELEASE);
  ARG_VAR(0);
  g_ppb_var->Release(arg0);
  printf("varRelease(%lld)\n", arg0.value.as_id);
}

void Handle_varToUtf8(Command* command) {
  TYPE_CHECK(TYPE_FUNC_VAR_TO_UTF8);
  ARG_VAR(0);
  ARG_VOIDP_CAST(1, uint32_t*);
  void* result = (void*)g_ppb_var->VarToUtf8(arg0, arg1);
  RegisterHandleVoidp(command->ret_handle, result);
  printf("varToUtf8(%lld, %p) => %p (%d)\n", arg0.value.as_id, arg1, result,
         command->ret_handle);
}
