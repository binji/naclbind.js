/* Copyright 2014 Ben Smith. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NB_ONE_FILE
#include "var.h"
#endif

#include <assert.h>
#include <string.h>

#ifndef NB_ONE_FILE
#include "error.h"
#include "interfaces.h"
#endif

void nb_var_addref(struct PP_Var var) {
  g_nb_ppb_var->AddRef(var);
}

void nb_var_release(struct PP_Var var) {
  g_nb_ppb_var->Release(var);
}

const char* nb_var_type_to_string(PP_VarType type) {
  switch (type) {
    case PP_VARTYPE_UNDEFINED:
      return "undefined";
    case PP_VARTYPE_NULL:
      return "null";
    case PP_VARTYPE_BOOL:
      return "bool";
    case PP_VARTYPE_INT32:
      return "int32";
    case PP_VARTYPE_DOUBLE:
      return "double";
    case PP_VARTYPE_STRING:
      return "string";
    case PP_VARTYPE_OBJECT:
      return "object";
    case PP_VARTYPE_ARRAY:
      return "array";
    case PP_VARTYPE_DICTIONARY:
      return "dictionary";
    case PP_VARTYPE_ARRAY_BUFFER:
      return "array buffer";
    case PP_VARTYPE_RESOURCE:
      return "resource";
    default:
      return "unknown";
  }
}

NB_Bool nb_var_check_type(struct PP_Var var, PP_VarType type) {
  return var.type == type ? NB_TRUE : NB_FALSE;
}

NB_Bool nb_var_check_type_with_error(struct PP_Var var, PP_VarType type) {
  if (var.type != type) {
    NB_VERROR("Expected var of type %s. Got %s.",
              nb_var_type_to_string(type),
              nb_var_type_to_string(var.type));
    return NB_FALSE;
  }

  return NB_TRUE;
}

struct PP_Var nb_var_string_create(const char* s, uint32_t len) {
  return g_nb_ppb_var->VarFromUtf8(s, len);
}

struct PP_Var nb_var_array_create(void) {
  return g_nb_ppb_var_array->Create();
}

uint32_t nb_var_array_length(struct PP_Var var) {
  assert(var.type == PP_VARTYPE_ARRAY);
  return g_nb_ppb_var_array->GetLength(var);
}

struct PP_Var nb_var_array_get(struct PP_Var var, uint32_t index) {
  assert(var.type == PP_VARTYPE_ARRAY);
  return g_nb_ppb_var_array->Get(var, index);
}

NB_Bool nb_var_array_set(struct PP_Var var,
                         uint32_t index,
                         struct PP_Var value) {
  assert(var.type == PP_VARTYPE_ARRAY);
  return g_nb_ppb_var_array->Set(var, index, value);
}

struct PP_Var nb_var_dict_create(void) {
  return g_nb_ppb_var_dictionary->Create();
}

struct PP_Var nb_var_dict_get(struct PP_Var var, const char* key) {
  assert(var.type == PP_VARTYPE_DICTIONARY);
  struct PP_Var key_var = g_nb_ppb_var->VarFromUtf8(key, strlen(key));
  struct PP_Var result = g_nb_ppb_var_dictionary->Get(var, key_var);
  nb_var_release(key_var);
  return result;
}

struct PP_Var nb_var_dict_get_var(struct PP_Var var, struct PP_Var key) {
  assert(var.type == PP_VARTYPE_DICTIONARY);
  return g_nb_ppb_var_dictionary->Get(var, key);
}

NB_Bool nb_var_dict_set(struct PP_Var var,
                        const char* key,
                        struct PP_Var value) {
  assert(var.type == PP_VARTYPE_DICTIONARY);
  struct PP_Var key_var = g_nb_ppb_var->VarFromUtf8(key, strlen(key));
  NB_Bool result = g_nb_ppb_var_dictionary->Set(var, key_var, value);
  nb_var_release(key_var);
  return result;
}

NB_Bool nb_var_dict_set_var(struct PP_Var var,
                            struct PP_Var key,
                            struct PP_Var value) {
  assert(var.type == PP_VARTYPE_DICTIONARY);
  return g_nb_ppb_var_dictionary->Set(var, key, value);
}

struct PP_Var nb_var_dict_get_keys(struct PP_Var var) {
  assert(var.type == PP_VARTYPE_DICTIONARY);
  return g_nb_ppb_var_dictionary->GetKeys(var);
}

NB_Bool nb_var_dict_has_key(struct PP_Var var, const char* key) {
  assert(var.type == PP_VARTYPE_DICTIONARY);
  struct PP_Var key_var = g_nb_ppb_var->VarFromUtf8(key, strlen(key));
  NB_Bool result = g_nb_ppb_var_dictionary->HasKey(var, key_var);
  nb_var_release(key_var);
  return result;
}

struct PP_Var nb_var_buffer_create(uint32_t length) {
  return g_nb_ppb_var_array_buffer->Create(length);
}

uint32_t nb_var_buffer_byte_length(struct PP_Var var) {
  assert(var.type == PP_VARTYPE_ARRAY_BUFFER);
  uint32_t length;
  NB_Bool result = g_nb_ppb_var_array_buffer->ByteLength(var, &length);
  assert(result);
  return length;
}

void nb_var_buffer_map(struct PP_Var var) {
  assert(var.type == PP_VARTYPE_ARRAY_BUFFER);
  g_nb_ppb_var_array_buffer->Map(var);
}

void nb_var_buffer_unmap(struct PP_Var var) {
  assert(var.type == PP_VARTYPE_ARRAY_BUFFER);
  g_nb_ppb_var_array_buffer->Unmap(var);
}

struct PP_Var nb_var_int64_create(int64_t value) {
  struct PP_Var result = nb_var_array_create();
  struct PP_Var tag = nb_var_string_create("long", 4);
  nb_var_array_set(result, 0, tag);
  nb_var_array_set(result, 1, PP_MakeInt32((int32_t)(value & 0xFFFFFFFF)));
  nb_var_array_set(result, 2, PP_MakeInt32((int32_t)(value >> 32)));
  nb_var_release(tag);
  return result;
}

NB_Bool nb_var_int8(struct PP_Var var, int8_t* out_value) {
  if (!nb_var_check_type_with_error(var, PP_VARTYPE_INT32)) {
    return NB_FALSE;
  }

  *out_value = var.value.as_int;
  return NB_TRUE;
}

NB_Bool nb_var_uint8(struct PP_Var var, uint8_t* out_value) {
  if (!nb_var_check_type_with_error(var, PP_VARTYPE_INT32)) {
    return NB_FALSE;
  }

  *out_value = var.value.as_int;
  return NB_TRUE;
}

NB_Bool nb_var_int16(struct PP_Var var, int16_t* out_value) {
  if (!nb_var_check_type_with_error(var, PP_VARTYPE_INT32)) {
    return NB_FALSE;
  }

  *out_value = var.value.as_int;
  return NB_TRUE;
}

NB_Bool nb_var_uint16(struct PP_Var var, uint16_t* out_value) {
  if (!nb_var_check_type_with_error(var, PP_VARTYPE_INT32)) {
    return NB_FALSE;
  }

  *out_value = var.value.as_int;
  return NB_TRUE;
}

NB_Bool nb_var_int32(struct PP_Var var, int32_t* out_value) {
  if (!nb_var_check_type_with_error(var, PP_VARTYPE_INT32)) {
    return NB_FALSE;
  }

  *out_value = var.value.as_int;
  return NB_TRUE;
}

NB_Bool nb_var_uint32(struct PP_Var var, uint32_t* out_value) {
  if (!nb_var_check_type_with_error(var, PP_VARTYPE_INT32)) {
    return NB_FALSE;
  }

  *out_value = var.value.as_int;
  return NB_TRUE;
}

NB_Bool nb_var_int64(struct PP_Var var, int64_t* out_value) {
  struct PP_Var low_var;
  struct PP_Var high_var;

  if (!nb_var_tagged_array_check(var, "long", 3)) {
    return NB_FALSE;
  }

  low_var = nb_var_array_get(var, 1);
  if (!nb_var_check_type_with_error(low_var, PP_VARTYPE_INT32)) {
    nb_var_release(low_var);
    return NB_FALSE;
  }

  high_var = nb_var_array_get(var, 2);
  if (!nb_var_check_type_with_error(high_var, PP_VARTYPE_INT32)) {
    nb_var_release(high_var);
    return NB_FALSE;
  }

  *out_value = ((int64_t)high_var.value.as_int << 32) | low_var.value.as_int;
  return NB_TRUE;
}

NB_Bool nb_var_uint64(struct PP_Var var, uint64_t* out_value) {
  int64_t signed_value;
  if (!nb_var_int64(var, &signed_value)) {
    return NB_FALSE;
  }

  *out_value = (uint64_t)signed_value;
  return NB_TRUE;
}

NB_Bool nb_var_float(struct PP_Var var, float* out_value) {
  if (!nb_var_check_type_with_error(var, PP_VARTYPE_DOUBLE)) {
    return NB_FALSE;
  }

  *out_value = var.value.as_double;
  return NB_TRUE;
}

NB_Bool nb_var_double(struct PP_Var var, double* out_value) {
  if (!nb_var_check_type_with_error(var, PP_VARTYPE_DOUBLE)) {
    return NB_FALSE;
  }

  *out_value = var.value.as_double;
  return NB_TRUE;
}

NB_Bool nb_var_string(struct PP_Var var,
                      const char** out_str,
                      uint32_t* out_length) {
  if (!nb_var_check_type_with_error(var, PP_VARTYPE_STRING)) {
    return NB_FALSE;
  }

  *out_str = g_nb_ppb_var->VarToUtf8(var, out_length);
  return *out_str != NULL;
}

NB_Bool nb_var_tagged_array(struct PP_Var var,
                            const char** out_tag,
                            uint32_t* out_tag_length,
                            uint32_t* out_array_length) {
  struct PP_Var tag;

  if (!nb_var_check_type_with_error(var, PP_VARTYPE_ARRAY)) {
    return NB_FALSE;
  }

  *out_array_length = nb_var_array_length(var);
  if (*out_array_length < 1) {
    NB_ERROR("Expected tagged array length to be at least 1.");
    return NB_FALSE;
  }

  tag = nb_var_array_get(var, 0);
  if (!nb_var_check_type_with_error(tag, PP_VARTYPE_STRING)) {
    nb_var_release(tag);
    return NB_FALSE;
  }

  if (!nb_var_string(tag, out_tag, out_tag_length)) {
    nb_var_release(tag);
    NB_ERROR("Unable to get tag string from tagged array.");
    return NB_FALSE;
  }

  nb_var_release(tag);
  return NB_TRUE;
}

NB_Bool nb_var_tagged_array_check(struct PP_Var var,
                                  const char* expected_tag,
                                  uint32_t expected_array_len) {
  const char* tag;
  uint32_t tag_len;
  uint32_t array_len;
  if (!nb_var_tagged_array(var, &tag, &tag_len, &array_len)) {
    return NB_FALSE;
  }

  if (array_len != expected_array_len) {
    NB_VERROR("Expected tagged array length to be %d, not %d.",
              expected_array_len,
              array_len);
    return NB_FALSE;
  }

  uint32_t expected_tag_len = strlen(expected_tag);
  if (tag_len != expected_tag_len || strncmp(tag, expected_tag, tag_len) != 0) {
    NB_VERROR("Expected tagged array tag to be \"%s\", not \"%.*s\".",
              expected_tag,
              tag_len,
              tag);
    return NB_FALSE;
  }

  return NB_TRUE;
}
