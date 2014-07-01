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

void* getVoidP(void** p) {
  return *p;
}

char getChar(char* p) {
  return *p;
}

int8_t getInt8(int8_t* p) {
  return *p;
}

uint8_t getUint8(uint8_t* p) {
  return *p;
}

int16_t getInt16(int16_t* p) {
  return *p;
}

uint16_t getUint16(uint16_t* p) {
  return *p;
}

int32_t getInt32(int32_t* p) {
  return *p;
}

uint32_t getUint32(uint32_t* p) {
  return *p;
}

long getLong(long* p) {
  return *p;
}

unsigned long getUlong(unsigned long* p) {
  return *p;
}

int64_t getInt64(int64_t* p) {
  return *p;
}

uint64_t getUint64(uint64_t* p) {
  return *p;
}

float getFloat32(float* p) {
  return *p;
}

double getFloat64(double* p) {
  return *p;
}

void setVoidP(void** p, void* x) {
  *p = x;
}

void setChar(char* p, char x) {
  *p = x;
}

void setInt8(int8_t* p, int8_t x) {
  *p = x;
}

void setUint8(uint8_t* p, uint8_t x) {
  *p = x;
}

void setInt16(int16_t* p, int16_t x) {
  *p = x;
}

void setUint16(uint16_t* p, uint16_t x) {
  *p = x;
}

void setInt32(int32_t* p, int32_t x) {
  *p = x;
}

void setUint32(uint32_t* p, uint32_t x) {
  *p = x;
}

void setLong(long* p, long x) {
  *p = x;
}

void setUlong(unsigned long* p, unsigned long x) {
  *p = x;
}

void setInt64(int64_t* p, int64_t x) {
  *p = x;
}

void setUint64(uint64_t* p, uint64_t x) {
  *p = x;
}

void setFloat32(float* p, float x) {
  *p = x;
}

void setFloat64(double* p, double x) {
  *p = x;
}

void* addVoidP(void* p, int32_t x) {
  return p + x;
}

int32_t addInt32(int32_t a, int32_t b) {
  return a + b;
}

uint32_t addUint32(uint32_t a, uint32_t b) {
  return a + b;
}

int64_t addInt64(int64_t a, int64_t b) {
  return a + b;
}

uint64_t addUint64(uint64_t a, uint64_t b) {
  return a + b;
}

float addFloat32(float a, float b) {
  return a + b;
}

double addFloat64(double a, double b) {
  return a + b;
}

void* subVoidP(void* a, int32_t b) {
  return a - b;
}

int32_t subInt32(int32_t a, int32_t b) {
  return a - b;
}

uint32_t subUint32(uint32_t a, uint32_t b) {
  return a - b;
}

int64_t subInt64(int64_t a, int64_t b) {
  return a - b;
}

uint64_t subUint64(uint64_t a, uint64_t b) {
  return a - b;
}

float subFloat32(float a, float b) {
  return a - b;
}

double subFloat64(double a, double b) {
  return a - b;
}

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

struct PP_Var arrayCreate() {
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

struct PP_Var dictCreate() {
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
