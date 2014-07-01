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

#ifndef BUILTIN_FUNCS_H_
#define BUILTIN_FUNCS_H_

#include <stdint.h>

#include <ppapi/c/pp_var.h>

void* getVoidP(void**);
char getChar(char*);
int8_t getInt8(int8_t*);
uint8_t getUint8(uint8_t*);
int16_t getInt16(int16_t*);
uint16_t getUint16(uint16_t*);
int32_t getInt32(int32_t*);
uint32_t getUint32(uint32_t*);
long getLong(long*);
unsigned long getUlong(unsigned long*);
int64_t getInt64(int64_t*);
uint64_t getUint64(uint64_t*);
float getFloat32(float*);
double getFloat64(double*);

void setVoidP(void**, void*);
void setChar(char*, char);
void setInt8(int8_t*, int8_t);
void setUint8(uint8_t*, uint8_t);
void setInt16(int16_t*, int16_t);
void setUint16(uint16_t*, uint16_t);
void setInt32(int32_t*, int32_t);
void setUint32(uint32_t*, uint32_t);
void setLong(long*, long);
void setUlong(unsigned long*, unsigned long);
void setInt64(int64_t*, int64_t);
void setUint64(uint64_t*, uint64_t);
void setFloat32(float*, float);
void setFloat64(double*, double);

void* addVoidP(void*, int32_t);
int32_t addInt32(int32_t, int32_t);
uint32_t addUint32(uint32_t, uint32_t);
int64_t addInt64(int64_t, int64_t);
uint64_t addUint64(uint64_t, uint64_t);
float addFloat32(float, float);
double addFloat64(double, double);

void* subVoidP(void*, int32_t);
int32_t subInt32(int32_t, int32_t);
uint32_t subUint32(uint32_t, uint32_t);
int64_t subInt64(int64_t, int64_t);
uint64_t subUint64(uint64_t, uint64_t);
float subFloat32(float, float);
double subFloat64(double, double);

void varAddRef(struct PP_Var);
void varRelease(struct PP_Var);
struct PP_Var varFromUtf8(const char*, uint32_t);
const char* varToUtf8(struct PP_Var, uint32_t*);

struct PP_Var arrayCreate();
struct PP_Var arrayGet(struct PP_Var, uint32_t);
int32_t arraySet(struct PP_Var, uint32_t, struct PP_Var);
uint32_t arrayGetLength(struct PP_Var);
int32_t arraySetLength(struct PP_Var, uint32_t);

struct PP_Var arrayBufferCreate(uint32_t);
int32_t arrayBufferByteLength(struct PP_Var, uint32_t*);
void* arrayBufferMap(struct PP_Var);
void arrayBufferUnmap(struct PP_Var);

struct PP_Var dictCreate();
struct PP_Var dictGet(struct PP_Var, struct PP_Var);
int32_t dictSet(struct PP_Var, struct PP_Var, struct PP_Var);
void dictDelete(struct PP_Var, struct PP_Var);
int32_t dictHasKey(struct PP_Var, struct PP_Var);


#endif  // BUILTIN_FUNCS_H_
