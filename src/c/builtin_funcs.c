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

#include "builtin_funcs.h"

#include <ppapi/c/pp_var.h>
#include <ppapi/c/ppb_var.h>
#include <ppapi/c/ppb_var_array.h>
#include <ppapi/c/ppb_var_array_buffer.h>
#include <ppapi/c/ppb_var_dictionary.h>

#include "interfaces.h"

void varAddRef(struct PP_Var var) {
  g_ppb_var->AddRef(var);
}

void varRelease(struct PP_Var var) {
  g_ppb_var->Release(var);
}

struct PP_Var varFromUtf8(const char* s, uint32_t len) {
  return g_ppb_var->VarFromUtf8(s, len);
}

const char* varToUtf8(struct PP_Var var, uint32_t* out_len) {
  return g_ppb_var->VarToUtf8(var, out_len);
}

struct PP_Var arrayCreate(void) {
  return g_ppb_var_array->Create();
}

struct PP_Var arrayGet(struct PP_Var var, uint32_t i) {
  return g_ppb_var_array->Get(var, i);
}

int32_t arraySet(struct PP_Var var, uint32_t i, struct PP_Var x) {
  return g_ppb_var_array->Set(var, i, x);
}

uint32_t arrayGetLength(struct PP_Var var) {
  return g_ppb_var_array->GetLength(var);
}

int32_t arraySetLength(struct PP_Var var, uint32_t i) {
  return g_ppb_var_array->SetLength(var, i);
}

struct PP_Var arrayBufferCreate(uint32_t size) {
  return g_ppb_var_array_buffer->Create(size);
}

int32_t arrayBufferByteLength(struct PP_Var var, uint32_t* out_len) {
  return g_ppb_var_array_buffer->ByteLength(var, out_len);
}

void* arrayBufferMap(struct PP_Var var) {
  return g_ppb_var_array_buffer->Map(var);
}

void arrayBufferUnmap(struct PP_Var var) {
  return g_ppb_var_array_buffer->Unmap(var);
}

struct PP_Var dictCreate(void) {
  return g_ppb_var_dictionary->Create();
}

struct PP_Var dictGet(struct PP_Var var, struct PP_Var k) {
  return g_ppb_var_dictionary->Get(var, k);
}

int32_t dictSet(struct PP_Var var, struct PP_Var k, struct PP_Var v) {
  return g_ppb_var_dictionary->Set(var, k, v);
}

void dictDelete(struct PP_Var var, struct PP_Var k) {
  return g_ppb_var_dictionary->Delete(var, k);
}

int32_t dictHasKey(struct PP_Var var, struct PP_Var k) {
  return g_ppb_var_dictionary->HasKey(var, k);
}
