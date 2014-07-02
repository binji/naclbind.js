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

#define FOREACH_PRIMITIVE(x) \
  x(VoidP, void*); \
  x(Char, char); \
  x(Int8, int8_t); \
  x(Uint8, uint8_t); \
  x(Int16, int16_t); \
  x(Uint16, uint16_t); \
  x(Int32, int32_t); \
  x(Uint32, uint32_t); \
  x(Long, long); \
  x(Ulong, unsigned long); \
  x(Int64, int64_t); \
  x(Uint64, uint64_t); \
  x(Float32, float); \
  x(Float64, double);

#define FOREACH_ADDSUB(x) \
  x(VoidP, void*, int32_t); \
  x(Int32, int32_t, int32_t); \
  x(Uint32, uint32_t, uint32_t); \
  x(Int64, int64_t, int64_t); \
  x(Uint64, uint64_t, uint64_t); \
  x(Float32, float, float); \
  x(Float64, double, double);

#define GET(name, type) inline type get##name(type* p) { return *p; }
#define SET(name, type) inline void set##name(type* p, type x) { *p = x; }
#define ADD(name, type1, type2) \
  inline type1 add##name(type1 a, type2 b) { return a + b; }
#define SUB(name, type1, type2) \
  inline type1 sub##name(type1 a, type2 b) { return a - b; }
#define LT(name, type) inline int lt##name(type a, type b) { return a < b; }
#define LTE(name, type) inline int lte##name(type a, type b) { return a <= b; }
#define GT(name, type) inline int gt##name(type a, type b) { return a > b; }
#define GTE(name, type) inline int gte##name(type a, type b) { return a >= b; }
#define EQ(name, type) inline int eq##name(type a, type b) { return a == b; }
#define NE(name, type) inline int ne##name(type a, type b) { return a != b; }

FOREACH_PRIMITIVE(GET)
FOREACH_PRIMITIVE(SET)
FOREACH_ADDSUB(ADD)
FOREACH_ADDSUB(SUB)
FOREACH_PRIMITIVE(LT)
FOREACH_PRIMITIVE(LTE)
FOREACH_PRIMITIVE(GT)
FOREACH_PRIMITIVE(GTE)
FOREACH_PRIMITIVE(EQ)
FOREACH_PRIMITIVE(NE)

#undef GET
#undef SET
#undef ADD
#undef SUB
#undef LT
#undef LTE
#undef GT
#undef GTE
#undef EQ
#undef NE
#undef FOREACH_PRIMITIVE
#undef FOREACH_ADDSUB

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
