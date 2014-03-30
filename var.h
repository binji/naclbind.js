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

#ifndef VAR_H_
#define VAR_H_

#include <ppapi/c/pp_var.h>

void AddRefVar(struct PP_Var*);
void ReleaseVar(struct PP_Var*);

void CreateArrayVar(struct PP_Var*);
uint32_t GetArrayVarLength(struct PP_Var*);
struct PP_Var GetArrayVar(struct PP_Var*, int32_t index);
void SetArrayVar(struct PP_Var*, int32_t index, struct PP_Var);

void CreateDictVar(struct PP_Var*);
struct PP_Var GetDictVar(struct PP_Var*, const char* key);
void SetDictVar(struct PP_Var*, const char* key, struct PP_Var);

bool GetVarInt8(struct PP_Var*, int8_t*);
bool GetVarUint8(struct PP_Var*, uint8_t*);
bool GetVarInt16(struct PP_Var*, int16_t*);
bool GetVarUint16(struct PP_Var*, uint16_t*);
bool GetVarInt32(struct PP_Var*, int32_t*);
bool GetVarUint32(struct PP_Var*, uint32_t*);
bool GetVarInt64(struct PP_Var*, int64_t*);
bool GetVarUint64(struct PP_Var*, uint64_t*);
bool GetVarFloat(struct PP_Var*, float*);
bool GetVarDouble(struct PP_Var*, double*);
bool GetVarString(struct PP_Var*, const char**, uint32_t* out_length);

#endif // VAR_H_
