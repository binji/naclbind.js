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

void nb_var_addref(struct PP_Var var) {
  g_ppb_var->AddRef(var);
}

void nb_var_release(struct PP_Var var) {
  g_ppb_var->Release(var);
}

struct PP_Var nb_var_string_create(const char* s, uint32_t len) {
  return g_ppb_var->VarFromUtf8(s, len);
}

struct PP_Var nb_var_array_create(void) {
  return g_ppb_var_array->Create();
}

uint32_t nb_var_array_length(struct PP_Var var) {
  assert(var.type == PP_VARTYPE_ARRAY);
  return g_ppb_var_array->GetLength(var);
}

struct PP_Var nb_var_array_get(struct PP_Var var, uint32_t index) {
  assert(var.type == PP_VARTYPE_ARRAY);
  return g_ppb_var_array->Get(var, index);
}

bool nb_var_array_set(struct PP_Var var, uint32_t index, struct PP_Var value) {
  assert(var.type == PP_VARTYPE_ARRAY);
  return g_ppb_var_array->Set(var, index, value);
}

struct PP_Var nb_var_dict_create(void) {
  return g_ppb_var_dictionary->Create();
}

struct PP_Var nb_var_dict_get(struct PP_Var var, const char* key) {
  assert(var.type == PP_VARTYPE_DICTIONARY);
  struct PP_Var key_var = g_ppb_var->VarFromUtf8(key, strlen(key));
  struct PP_Var result = g_ppb_var_dictionary->Get(var, key_var);
  nb_var_release(key_var);
  return result;
}

struct PP_Var nb_var_dict_get_var(struct PP_Var var, struct PP_Var key) {
  assert(var.type == PP_VARTYPE_DICTIONARY);
  return g_ppb_var_dictionary->Get(var, key);
}

bool nb_var_dict_set(struct PP_Var var, const char* key, struct PP_Var value) {
  assert(var.type == PP_VARTYPE_DICTIONARY);
  struct PP_Var key_var = g_ppb_var->VarFromUtf8(key, strlen(key));
  bool result = g_ppb_var_dictionary->Set(var, key_var, value);
  nb_var_release(key_var);
  return result;
}

bool nb_var_dict_set_var(struct PP_Var var, struct PP_Var key,
                         struct PP_Var value) {
  assert(var.type == PP_VARTYPE_DICTIONARY);
  return g_ppb_var_dictionary->Set(var, key, value);
}

struct PP_Var nb_var_dict_get_keys(struct PP_Var var) {
  assert(var.type == PP_VARTYPE_DICTIONARY);
  return g_ppb_var_dictionary->GetKeys(var);
}

bool nb_var_dict_has_key(struct PP_Var var, const char* key) {
  assert(var.type == PP_VARTYPE_DICTIONARY);
  struct PP_Var key_var = g_ppb_var->VarFromUtf8(key, strlen(key));
  bool result = g_ppb_var_dictionary->HasKey(var, key_var);
  nb_var_release(key_var);
  return result;
}

bool nb_var_int8(struct PP_Var var, int8_t* out_value) {
  if (var.type != PP_VARTYPE_INT32) {
    VERROR("expected var of type INT32. Got %d.", var.type);
    return FALSE;
  }

  *out_value = var.value.as_int;
  return TRUE;
}

bool nb_var_uint8(struct PP_Var var, uint8_t* out_value) {
  if (var.type != PP_VARTYPE_INT32) {
    VERROR("expected var of type INT32. Got %d.", var.type);
    return FALSE;
  }

  *out_value = var.value.as_int;
  return TRUE;
}

bool nb_var_int16(struct PP_Var var, int16_t* out_value) {
  if (var.type != PP_VARTYPE_INT32) {
    VERROR("expected var of type INT32. Got %d.", var.type);
    return FALSE;
  }

  *out_value = var.value.as_int;
  return TRUE;
}

bool nb_var_uint16(struct PP_Var var, uint16_t* out_value) {
  if (var.type != PP_VARTYPE_INT32) {
    VERROR("expected var of type INT32. Got %d.", var.type);
    return FALSE;
  }

  *out_value = var.value.as_int;
  return TRUE;
}

bool nb_var_int32(struct PP_Var var, int32_t* out_value) {
  if (var.type != PP_VARTYPE_INT32) {
    VERROR("expected var of type INT32. Got %d.", var.type);
    return FALSE;
  }

  *out_value = var.value.as_int;
  return TRUE;
}

bool nb_var_uint32(struct PP_Var var, uint32_t* out_value) {
  if (var.type != PP_VARTYPE_INT32) {
    VERROR("expected var of type INT32. Got %d.", var.type);
    return FALSE;
  }

  *out_value = var.value.as_int;
  return TRUE;
}

bool nb_var_int64(struct PP_Var var, int64_t* out_value) {
  // TODO: figure out how best to encode int64
  return FALSE;
}

bool nb_var_uint64(struct PP_Var var, uint64_t* out_value) {
  // TODO: figure out how best to encode int64
  return FALSE;
}

bool nb_var_float(struct PP_Var var, float* out_value) {
  if (var.type != PP_VARTYPE_DOUBLE) {
    VERROR("expected var of type DOUBLE. Got %d.", var.type);
    return FALSE;
  }

  *out_value = var.value.as_double;
  return TRUE;
}

bool nb_var_double(struct PP_Var var, double* out_value) {
  if (var.type != PP_VARTYPE_DOUBLE) {
    VERROR("expected var of type DOUBLE. Got %d.", var.type);
    return FALSE;
  }

  *out_value = var.value.as_double;
  return TRUE;
}

bool nb_var_string(struct PP_Var var, const char** out_str,
                   uint32_t* out_length) {
  if (var.type != PP_VARTYPE_STRING) {
    VERROR("expected var of type STRING. Got %d.", var.type);
    return FALSE;
  }

  *out_str = g_ppb_var->VarToUtf8(var, out_length);
  return *out_str != NULL;
}
