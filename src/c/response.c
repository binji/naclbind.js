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
#include "response.h"
#endif

static NB_Bool nb_expect_key(struct PP_Var var,
                             const char* key,
                             struct PP_Var* out_value);
static NB_Bool nb_optional_key(struct PP_Var var,
                               const char* key,
                               struct PP_Var* out_value);
static NB_Bool nb_response_parse_id(struct NB_Response* response,
                                    struct PP_Var var);
static NB_Bool nb_response_parse_cb_id(struct NB_Response* response,
                                       struct PP_Var var);
static NB_Bool nb_response_parse_values(struct NB_Response* response,
                                        struct PP_Var var);

struct NB_Response {
  struct PP_Var var;

  /* The following are only used when parsing a response, not creating one. */
  int id;
  int cb_id;
  struct PP_Var* values;
  uint32_t values_count;
};

struct NB_Response* nb_response_create(int id) {
  struct NB_Response* response = calloc(1, sizeof(struct NB_Response));
  struct PP_Var values;

  response->var = nb_var_dict_create();
  if (!nb_var_dict_set(response->var, "id", PP_MakeInt32(id))) {
    NB_VERROR("nb_response_create failed to set \"id\" to %d.", id);
    goto cleanup;
  }

  values = nb_var_array_create();
  if (!nb_var_dict_set(response->var, "values", values)) {
    NB_ERROR("nb_response_create failed to create \"values\" array.");
    nb_var_release(values);
    goto cleanup;
  }

  nb_var_release(values);
  return response;

cleanup:
  nb_var_release(response->var);
  free(response);
  return NULL;
}

NB_Bool nb_response_set_cb_id(struct NB_Response* response, int cb_id) {
  if (!nb_var_dict_set(response->var, "cbId", PP_MakeInt32(cb_id))) {
    NB_VERROR("nb_response_set_cb_id failed to set \"cbId\" to %d.", cb_id);
    return NB_FALSE;
  }

  return NB_TRUE;
}

void nb_response_destroy(struct NB_Response* response) {
  uint32_t i;
  assert(response != NULL);

  for (i = 0; i < response->values_count; ++i) {
    nb_var_release(response->values[i]);
  }

  nb_var_release(response->var);
  free(response);
}

NB_Bool nb_response_set_value(struct NB_Response* response,
                              int i,
                              struct PP_Var value) {
  NB_Bool result = NB_FALSE;
  struct PP_Var values = nb_var_dict_get(response->var, "values");

  if (!nb_var_array_set(values, i, value)) {
    NB_VERROR("nb_response_set_value(%d, %s) failed.", i,
              nb_var_type_to_string(value.type));
    goto cleanup;
  }

  result = NB_TRUE;

cleanup:
  nb_var_release(values);
  return result;
}

struct PP_Var nb_response_get_var(struct NB_Response* response) {
  nb_var_addref(response->var);
  return response->var;
}

NB_Bool nb_response_set_error(struct NB_Response* response,
                              int failed_command_idx) {
  if (!nb_var_dict_set(response->var, "error",
                       PP_MakeInt32(failed_command_idx))) {
    NB_VERROR("nb_response_set_error(%d) failed.", failed_command_idx);
    return NB_FALSE;
  }

  return NB_TRUE;
}

struct NB_Response* nb_response_parse(struct PP_Var var) {
  struct NB_Response* response = calloc(1, sizeof(struct NB_Response));
  if (!(nb_var_check_type_with_error(var, PP_VARTYPE_DICTIONARY) &&
        nb_response_parse_id(response, var) &&
        nb_response_parse_cb_id(response, var) &&
        nb_response_parse_values(response, var))) {
    nb_response_destroy(response);
    return NULL;
  }

  return response;
}

static NB_Bool nb_response_parse_id(struct NB_Response* response,
                                    struct PP_Var var) {
  NB_Bool result = NB_FALSE;
  struct PP_Var id = PP_MakeUndefined();

  if (!nb_expect_key(var, "id", &id)) {
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(id, PP_VARTYPE_INT32)) {
    goto cleanup;
  }

  if (id.value.as_int <= 0) {
    NB_VERROR("Expected response id to be > 0. Got %d", id.value.as_int);
    goto cleanup;
  }

  response->id = id.value.as_int;
  result = NB_TRUE;
cleanup:
  nb_var_release(id);
  return result;
}

static NB_Bool nb_response_parse_cb_id(struct NB_Response* response,
                                       struct PP_Var var) {
  NB_Bool result = NB_FALSE;
  struct PP_Var cb_id = PP_MakeUndefined();

  if (!nb_optional_key(var, "cbId", &cb_id)) {
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(cb_id, PP_VARTYPE_INT32)) {
    goto cleanup;
  }

  if (cb_id.value.as_int <= 0) {
    NB_VERROR("Expected response cbId to be > 0. Got %d", cb_id.value.as_int);
    goto cleanup;
  }

  response->cb_id = cb_id.value.as_int;
  result = NB_TRUE;
cleanup:
  nb_var_release(cb_id);
  return result;
}

static NB_Bool nb_response_parse_values(struct NB_Response* response,
                                        struct PP_Var var) {
  NB_Bool result = NB_FALSE;
  struct PP_Var values_var = PP_MakeUndefined();
  struct PP_Var* values = NULL;
  uint32_t i, len;

  if (!nb_expect_key(var, "values", &values_var)) {
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(values_var, PP_VARTYPE_ARRAY)) {
    goto cleanup;
  }

  len = nb_var_array_length(values_var);
  values = nb_calloc_list(len, sizeof(struct PP_Var));
  for (i = 0; i < len; ++i) {
    values[i] = nb_var_array_get(values_var, i);
  }

  response->values = values;
  response->values_count = len;

  result = NB_TRUE;
  values = NULL; /* Pass ownership to the response */
cleanup:
  free(values);
  nb_var_release(values_var);
  return result;
}

int nb_response_id(struct NB_Response* response) {
  return response->id;
}

int nb_response_cb_id(struct NB_Response* response) {
  return response->cb_id;
}

int nb_response_values_count(struct NB_Response* response) {
  assert(response != NULL);
  return response->values_count;
}

struct PP_Var nb_response_value(struct NB_Response* response, int index) {
  assert(response != NULL);
  assert(index >= 0 && index < response->values_count);
  return response->values[index];
}
