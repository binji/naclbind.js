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
bool nb_message_command_run(struct Message* message, int command_idx);

static bool nb_request_set_handles(struct Message* message);
static bool nb_request_run_commands(struct Message* message);
static bool nb_request_get_handles(struct Message* message,
                                   struct PP_Var* out_response);
static void nb_request_destroy_handles(struct Message* message);

bool nb_request_run(struct PP_Var request, struct PP_Var* out_response) {
  bool result = FALSE;
  struct Message* message = NULL;

  message = nb_message_create(request);
  if (message == NULL) {
    ERROR("nb_message_create() failed.");
    goto cleanup;
  }

  if (!nb_request_set_handles(message)) {
    ERROR("nb_request_set_handles() failed.");
    goto cleanup;
  }

  if (!nb_request_run_commands(message)) {
    ERROR("nb_request_run_commands() failed.");
    goto cleanup;
  }

  if (!nb_request_get_handles(message, out_response)) {
    ERROR("nb_request_get_handles() failed.");
    goto cleanup;
  }

  nb_request_destroy_handles(message);
  result = TRUE;

cleanup:
  if (message != NULL) {
    nb_message_destroy(message);
  }

  return result;
}

bool nb_request_set_handles(struct Message* message) {
  bool result = FALSE;
  int sethandles_count = nb_message_sethandles_count(message);
  int i;
  struct PP_Var value = PP_MakeUndefined();

  for (i = 0; i < sethandles_count; ++i) {
    Handle handle;
    nb_message_sethandle(message, i, &handle, &value);

    switch (value.type) {
      case PP_VARTYPE_INT32:
        if (!nb_handle_register_int32(handle, value.value.as_int)) {
          VERROR("nb_handle_register_int32(%d, %d) failed, i=%d.",
                 handle, value.value.as_int, i);
          goto cleanup;
        }
        break;

      case PP_VARTYPE_DOUBLE:
        if (!nb_handle_register_double(handle, value.value.as_double)) {
          VERROR("nb_handle_register_double(%d, %g) failed, i=%d.",
                 handle, value.value.as_double, i);
          goto cleanup;
        }
        break;

      case PP_VARTYPE_STRING: {
        if (!nb_handle_register_var(handle, value)) {
          VERROR("nb_handle_register_var(%d, %s) failed, i=%d.",
                 handle, nb_var_type_to_string(value.type), i);
          goto cleanup;
        }
        break;
      }

      case PP_VARTYPE_NULL:
        if (!nb_handle_register_voidp(handle, NULL)) {
          VERROR("nb_handle_register_voidp(%d, NULL) failed, i=%d.", handle, i);
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

  result = TRUE;

cleanup:
  nb_var_release(value);

  return TRUE;
}

bool nb_request_run_commands(struct Message* message) {
  int commands_count = nb_message_commands_count(message);
  int i;

  for (i = 0; i < commands_count; ++i) {
    if (!nb_message_command_run(message, i)) {
      VERROR("nb_message_command_run(%d) failed.", i);
      return FALSE;
    }
  }

  return TRUE;
}

bool nb_request_get_handles(struct Message* message,
                            struct PP_Var* out_response) {
  bool result = FALSE;
  int gethandles_count = nb_message_gethandles_count(message);
  int message_id = nb_message_id(message);
  struct PP_Var response = PP_MakeUndefined();
  struct PP_Var values = PP_MakeUndefined();
  int i;

  response = nb_var_dict_create();
  if (!nb_var_dict_set(response, "id", PP_MakeInt32(message_id))) {
    VERROR("nb_var_dict_set(\"id\", %d) failed.", message_id);
    goto cleanup;
  }

  values = nb_var_array_create();

  for (i = 0; i < gethandles_count; ++i) {
    Handle handle = nb_message_gethandle(message, i);
    struct PP_Var value = PP_MakeUndefined();

    if (!nb_handle_convert_to_var(handle, &value)) {
      VERROR("nb_handle_convert_to_var(%d, <value>) failed.", handle);
      goto cleanup;
    }

    if (!nb_var_array_set(values, i, value)) {
      VERROR("nb_var_array_set(values, %d, %s) failed.",
             i, nb_var_type_to_string(value.type));
      nb_var_release(value);
      goto cleanup;
    }

    nb_var_release(value);
  }

  if (!nb_var_dict_set(response, "values", values)) {
    ERROR("nb_var_dict_set(\"values\", ...) failed.");
    goto cleanup;
  }

  *out_response = response;
  result = TRUE;

  /* Clear the response so it isn't released twice. */
  memset(&response, 0, sizeof(struct PP_Var));

cleanup:
  nb_var_release(values);
  nb_var_release(response);
  return result;
}

void nb_request_destroy_handles(struct Message* message) {
  int destroyhandles_count = nb_message_destroyhandles_count(message);
  Handle* handles = alloca(destroyhandles_count * sizeof(Handle));
  int i;

  for (i = 0; i < destroyhandles_count; ++i) {
    handles[i] = nb_message_destroyhandle(message, i);
  }

  nb_handle_destroy_many(handles, destroyhandles_count);
}
