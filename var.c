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

#include "var.h"

#include <assert.h>
#include <string.h>

#include "error.h"
#include "interfaces.h"

void AddRefVar(struct PP_Var* var) {
  g_ppb_var->AddRef(*var);
}

void ReleaseVar(struct PP_Var* var) {
  g_ppb_var->Release(*var);
}

void CreateArrayVar(struct PP_Var* var) {
  *var = g_ppb_var_array->Create();
}

uint32_t GetArrayVarLength(struct PP_Var* var) {
  assert(var->type == PP_VARTYPE_ARRAY);
  return g_ppb_var_array->GetLength(*var);
}

struct PP_Var GetArrayVar(struct PP_Var* var, int32_t index) {
  assert(var->type == PP_VARTYPE_ARRAY);
  return g_ppb_var_array->Get(*var, index);
}

void SetArrayVar(struct PP_Var* var, int32_t index, struct PP_Var value) {
  assert(var->type == PP_VARTYPE_ARRAY);
  g_ppb_var_array->Set(*var, index, value);
}

void CreateDictVar(struct PP_Var* var) {
  *var = g_ppb_var_dictionary->Create();
}

struct PP_Var GetDictVar(struct PP_Var* var, const char* key) {
  assert(var->type == PP_VARTYPE_DICTIONARY);
  PP_Var key_var = g_ppb_var->VarFromUtf8(key, strlen(key));
  PP_Var result = g_ppb_var_dictionary->Get(*var, key_var);
  ReleaseVar(&key_var);
  return result;
}

void SetDictVar(struct PP_Var* var, const char* key, struct PP_Var value) {
  assert(var->type == PP_VARTYPE_DICTIONARY);
  PP_Var key_var = g_ppb_var->VarFromUtf8(key, strlen(key));
  g_ppb_var_dictionary->Set(*var, key_var, value);
  ReleaseVar(&key_var);
}

bool GetVarInt8(struct PP_Var* var, int8_t* out_value) {
  if (var->type != PP_VARTYPE_INT32) {
    VERROR("expected var of type INT32. Got %d.", var->type);
    return false;
  }

  *out_value = var->value.as_int;
  return true;
}

bool GetVarUint8(struct PP_Var* var, uint8_t* out_value) {
  if (var->type != PP_VARTYPE_INT32) {
    VERROR("expected var of type INT32. Got %d.", var->type);
    return false;
  }

  *out_value = var->value.as_int;
  return true;
}

bool GetVarInt16(struct PP_Var* var, int16_t* out_value) {
  if (var->type != PP_VARTYPE_INT32) {
    VERROR("expected var of type INT32. Got %d.", var->type);
    return false;
  }

  *out_value = var->value.as_int;
  return true;
}

bool GetVarUint16(struct PP_Var* var, uint16_t* out_value) {
  if (var->type != PP_VARTYPE_INT32) {
    VERROR("expected var of type INT32. Got %d.", var->type);
    return false;
  }

  *out_value = var->value.as_int;
  return true;
}

bool GetVarInt32(struct PP_Var* var, int32_t* out_value) {
  if (var->type != PP_VARTYPE_INT32) {
    VERROR("expected var of type INT32. Got %d.", var->type);
    return false;
  }

  *out_value = var->value.as_int;
  return true;
}

bool GetVarUint32(struct PP_Var* var, uint32_t* out_value) {
  if (var->type != PP_VARTYPE_INT32) {
    VERROR("expected var of type INT32. Got %d.", var->type);
    return false;
  }

  *out_value = var->value.as_int;
  return true;
}

bool GetVarInt64(struct PP_Var* var, int64_t* out_value) {
  // TODO: figure out how best to encode int64
  return false;
}

bool GetVarUint64(struct PP_Var* var, uint64_t* out_value) {
  // TODO: figure out how best to encode int64
  return false;
}

bool GetVarFloat(struct PP_Var* var, float* out_value) {
  if (var->type != PP_VARTYPE_DOUBLE) {
    VERROR("expected var of type DOUBLE. Got %d.", var->type);
    return false;
  }

  *out_value = var->value.as_double;
  return true;
}

bool GetVarDouble(struct PP_Var* var, double* out_value) {
  if (var->type != PP_VARTYPE_DOUBLE) {
    VERROR("expected var of type DOUBLE. Got %d.", var->type);
    return false;
  }

  *out_value = var->value.as_double;
  return true;
}

bool GetVarString(struct PP_Var* var,
                  const char** out_str, uint32_t* out_length) {
  if (var->type != PP_VARTYPE_STRING) {
    VERROR("expected var of type STRING. Got %d.", var->type);
    return false;
  }

  *out_str = g_ppb_var->VarToUtf8(*var, out_length);
  return *out_str != NULL;
}
