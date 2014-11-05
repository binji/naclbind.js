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

#ifndef NB_ONE_FILE
#include "message.h"
#endif

struct NB_Response {
  struct PP_Var var;
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

void nb_response_destroy(struct NB_Response* response) {
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
