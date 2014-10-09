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
#include "run.h"
#endif

#include <alloca.h>

#ifndef NB_ONE_FILE
#include "handle.h"
#include "message.h"
#endif

/* This function is defined by the generated code. */
NB_Bool nb_message_command_run(struct NB_Message* message, int command_idx);

static NB_Bool nb_request_set_handles(struct NB_Message* message);
static NB_Bool nb_request_run_commands(struct NB_Message* message);
static NB_Bool nb_request_get_handles(struct NB_Message* message,
                                      struct PP_Var* out_response);
static void nb_request_destroy_handles(struct NB_Message* message);

NB_Bool nb_request_run(struct PP_Var request, struct PP_Var* out_response) {
  NB_Bool result = NB_FALSE;
  struct NB_Message* message = NULL;

  message = nb_message_create(request);
  if (message == NULL) {
    NB_ERROR("nb_message_create() failed.");
    goto cleanup;
  }

  if (!nb_request_set_handles(message)) {
    NB_ERROR("nb_request_set_handles() failed.");
    goto cleanup;
  }

  if (!nb_request_run_commands(message)) {
    NB_ERROR("nb_request_run_commands() failed.");
    goto cleanup;
  }

  if (!nb_request_get_handles(message, out_response)) {
    NB_ERROR("nb_request_get_handles() failed.");
    goto cleanup;
  }

  nb_request_destroy_handles(message);
  result = NB_TRUE;

cleanup:
  if (message != NULL) {
    nb_message_destroy(message);
  }

  return result;
}

NB_Bool nb_request_set_handles(struct NB_Message* message) {
  NB_Bool result = NB_FALSE;
  int sethandles_count = nb_message_sethandles_count(message);
  int i;
  struct PP_Var value = PP_MakeUndefined();

  for (i = 0; i < sethandles_count; ++i) {
    NB_Handle handle;
    nb_message_sethandle(message, i, &handle, &value);

    switch (value.type) {
      case PP_VARTYPE_INT32:
        if (!nb_handle_register_int32(handle, value.value.as_int)) {
          NB_VERROR("nb_handle_register_int32(%d, %d) failed, i=%d.",
                    handle,
                    value.value.as_int,
                    i);
          goto cleanup;
        }
        break;

      case PP_VARTYPE_DOUBLE:
        if (!nb_handle_register_double(handle, value.value.as_double)) {
          NB_VERROR("nb_handle_register_double(%d, %g) failed, i=%d.",
                    handle,
                    value.value.as_double,
                    i);
          goto cleanup;
        }
        break;

      case PP_VARTYPE_STRING: {
        if (!nb_handle_register_var(handle, value)) {
          NB_VERROR("nb_handle_register_var(%d, %s) failed, i=%d.",
                    handle,
                    nb_var_type_to_string(value.type),
                    i);
          goto cleanup;
        }
        break;
      }

      case PP_VARTYPE_ARRAY: {
        int64_t num;
        if (!nb_var_int64(value, &num)) {
          NB_VERROR("nb_var_int64(%d, %s) failed, i=%d.",
                    handle,
                    nb_var_type_to_string(value.type),
                    i);
          goto cleanup;
        }

        if (!nb_handle_register_int64(handle, num)) {
          NB_VERROR("nb_handle_register_int64(%d, %lld) failed, i=%d.",
                    handle,
                    num,
                    i);
          goto cleanup;
        }
        break;
      }

      case PP_VARTYPE_NULL:
        if (!nb_handle_register_voidp(handle, NULL)) {
          NB_VERROR(
              "nb_handle_register_voidp(%d, NULL) failed, i=%d.", handle, i);
          goto cleanup;
        }
        break;

      default:
        /* This shouldn't happen; we've already validated the message before
         * getting here. */
        assert(!"Unsupported value type.");
        goto cleanup;
    }

    nb_var_release(value);
    /* Clear value so it isn't released twice */
    memset(&value, 0, sizeof(struct PP_Var));
  }

  result = NB_TRUE;

cleanup:
  nb_var_release(value);

  return result;
}

NB_Bool nb_request_run_commands(struct NB_Message* message) {
  int commands_count = nb_message_commands_count(message);
  int i;

  for (i = 0; i < commands_count; ++i) {
    if (!nb_message_command_run(message, i)) {
      NB_VERROR("nb_message_command_run(%d) failed.", i);
      return NB_FALSE;
    }
  }

  return NB_TRUE;
}

NB_Bool nb_request_get_handles(struct NB_Message* message,
                               struct PP_Var* out_response) {
  NB_Bool result = NB_FALSE;
  int gethandles_count = nb_message_gethandles_count(message);
  int message_id = nb_message_id(message);
  struct PP_Var response = PP_MakeUndefined();
  struct PP_Var values = PP_MakeUndefined();
  int i;

  response = nb_var_dict_create();
  if (!nb_var_dict_set(response, "id", PP_MakeInt32(message_id))) {
    NB_VERROR("nb_var_dict_set(\"id\", %d) failed.", message_id);
    goto cleanup;
  }

  values = nb_var_array_create();

  for (i = 0; i < gethandles_count; ++i) {
    NB_Handle handle = nb_message_gethandle(message, i);
    struct PP_Var value = PP_MakeUndefined();

    if (!nb_handle_convert_to_var(handle, &value)) {
      NB_VERROR("nb_handle_convert_to_var(%d, <value>) failed.", handle);
      goto cleanup;
    }

    if (!nb_var_array_set(values, i, value)) {
      NB_VERROR("nb_var_array_set(values, %d, %s) failed.",
                i,
                nb_var_type_to_string(value.type));
      nb_var_release(value);
      goto cleanup;
    }

    nb_var_release(value);
  }

  if (!nb_var_dict_set(response, "values", values)) {
    NB_ERROR("nb_var_dict_set(\"values\", ...) failed.");
    goto cleanup;
  }

  *out_response = response;
  result = NB_TRUE;

  /* Clear the response so it isn't released twice. */
  memset(&response, 0, sizeof(struct PP_Var));

cleanup:
  nb_var_release(values);
  nb_var_release(response);
  return result;
}

void nb_request_destroy_handles(struct NB_Message* message) {
  int destroyhandles_count = nb_message_destroyhandles_count(message);
  NB_Handle* handles = alloca(destroyhandles_count * sizeof(NB_Handle));
  int i;

  for (i = 0; i < destroyhandles_count; ++i) {
    handles[i] = nb_message_destroyhandle(message, i);
  }

  nb_handle_destroy_many(handles, destroyhandles_count);
}
