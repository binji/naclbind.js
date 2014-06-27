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

#ifndef HANDLE_H_
#define HANDLE_H_

#include <stdint.h>

#include <ppapi/c/pp_var.h>

#include "bool.h"
#include "type.h"

typedef union {
  int8_t int8;
  uint8_t uint8;
  int16_t int16;
  uint16_t uint16;
  int32_t int32;
  uint32_t uint32;
  int64_t int64;
  uint64_t uint64;
  float float32;
  double float64;
  void* voidp;
  struct PP_Var var;
} HandleValue;

typedef struct {
  Type type;
  HandleValue value;
  // PP_Var strings are not guaranteed to be NULL-terminated, so if we want to
  // use it as a C string, we have to allocate space for a NULL and remember to
  // free it later.
  //
  // This field will be non-NULL when type == TYPE_STRING and GetHandleCharp()
  // has been called. The memory will be free'd in DestroyHandle.
  char* string_value;
} HandleObject;

typedef int32_t Handle;

bool RegisterHandle(Handle, Type, HandleValue);
bool RegisterHandleInt8(Handle, int8_t);
bool RegisterHandleUint8(Handle, uint8_t);
bool RegisterHandleInt16(Handle, int16_t);
bool RegisterHandleUint16(Handle, uint16_t);
bool RegisterHandleInt32(Handle, int32_t);
bool RegisterHandleUint32(Handle, uint32_t);
bool RegisterHandleInt64(Handle, int64_t);
bool RegisterHandleUint64(Handle, uint64_t);
bool RegisterHandleFloat(Handle, float);
bool RegisterHandleDouble(Handle, double);
bool RegisterHandleVoidp(Handle, void*);
bool RegisterHandleVar(Handle, struct PP_Var);
bool GetHandle(Handle, HandleObject*);
bool GetHandleInt8(Handle, int8_t*);
bool GetHandleUint8(Handle, uint8_t*);
bool GetHandleInt16(Handle, int16_t*);
bool GetHandleUint16(Handle, uint16_t*);
bool GetHandleInt32(Handle, int32_t*);
bool GetHandleUint32(Handle, uint32_t*);
bool GetHandleInt64(Handle, int64_t*);
bool GetHandleUint64(Handle, uint64_t*);
bool GetHandleFloat(Handle, float*);
bool GetHandleDouble(Handle, double*);
bool GetHandleVoidp(Handle, void**);
bool GetHandleCharp(Handle, char**);
bool GetHandleVar(Handle, struct PP_Var*);
void DestroyHandle(Handle);
void DestroyHandles(Handle*, int32_t handle_count);

bool HandleToVar(Handle, struct PP_Var*);

#endif  // HANDLE_H_
