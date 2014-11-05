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
#include "response.h"
#endif

/* This function is defined by the generated code. */
NB_Bool nb_message_command_run(struct NB_Message* message, int command_idx);

static NB_Bool nb_request_set_handles(struct NB_Message* message);
static NB_Bool nb_request_run_commands(struct NB_Message* message,
                                       int* out_failed_command_idx);
static NB_Bool nb_request_get_handles(struct NB_Message* message,
                                      struct NB_Response* response);
static void nb_request_destroy_handles(struct NB_Message* message);

NB_Bool nb_request_run(struct PP_Var request,
                       struct PP_Var* out_response_var) {
  NB_Bool result = NB_FALSE;
  struct NB_Message* message = NULL;
  struct NB_Response* response = NULL;
  int failed_command_idx = -1;

  message = nb_message_create(request);
  if (message == NULL) {
    NB_ERROR("nb_message_create() failed.");
    goto cleanup;
  }

  response = nb_response_create(nb_message_id(message));
  if (response == NULL) {
    goto cleanup;
  }

  if (!nb_request_set_handles(message)) {
    NB_ERROR("nb_request_set_handles() failed.");
    goto cleanup;
  }

  result = NB_TRUE;

  if (!nb_request_run_commands(message, &failed_command_idx)) {
    NB_ERROR("nb_request_run_commands() failed.");
    result = NB_FALSE;
  }

  if (!nb_request_get_handles(message, response)) {
    NB_ERROR("nb_request_get_handles() failed.");
    result = NB_FALSE;
  }

  nb_request_destroy_handles(message);

cleanup:
  if (message != NULL) {
    nb_message_destroy(message);
  }

  if (response) {
    if (!result) {
      nb_response_set_error(response, failed_command_idx);
    }

    *out_response_var = nb_response_get_var(response);
    nb_response_destroy(response);
  } else {
    NB_ERROR("Can't set response error because response wasn't created!");
    *out_response_var = PP_MakeUndefined();
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

NB_Bool nb_request_run_commands(struct NB_Message* message,
                                int* out_failed_command_idx) {
  int commands_count = nb_message_commands_count(message);
  int i;

  for (i = 0; i < commands_count; ++i) {
    if (!nb_message_command_run(message, i)) {
      *out_failed_command_idx = i;
      NB_VERROR("nb_message_command_run(%d) failed.", i);
      return NB_FALSE;
    }
  }

  return NB_TRUE;
}

NB_Bool nb_request_get_handles(struct NB_Message* message,
                               struct NB_Response* response) {
  NB_Bool result = NB_TRUE;
  int gethandles_count = nb_message_gethandles_count(message);
  int i;

  for (i = 0; i < gethandles_count; ++i) {
    NB_Handle handle = nb_message_gethandle(message, i);
    struct PP_Var value = PP_MakeUndefined();

    if (!nb_handle_convert_to_var(handle, &value)) {
      NB_VERROR("nb_handle_convert_to_var(%d, <value>) failed.", handle);
      result = NB_FALSE;
    }

    if (!nb_response_set_value(response, i, value)) {
      result = NB_FALSE;
    }

    nb_var_release(value);
  }

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
