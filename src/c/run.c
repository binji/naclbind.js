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
#include "interfaces.h"
#include "request.h"
#include "response.h"
#include "var.h"
#endif

/* This function is defined by the generated code. */
NB_Bool nb_request_command_run(struct NB_Queue* message_queue,
                               struct NB_Request* request,
                               int command_idx);

static NB_Bool nb_request_set_handles(struct NB_Request* request);
static NB_Bool nb_request_run_commands(struct NB_Queue* message_queue,
                                       struct NB_Request* request,
                                       int* out_failed_command_idx);
static NB_Bool nb_request_get_handles(struct NB_Request* request,
                                      struct NB_Response* response);
static void nb_request_destroy_handles(struct NB_Request* request);

void nb_run_message_loop(struct NB_Queue* message_queue) {
  while (1) {
    struct PP_Var request = nb_queue_dequeue(message_queue);
    struct PP_Var response = PP_MakeUndefined();

    nb_request_run(message_queue, request, &response);
    g_nb_ppb_messaging->PostMessage(g_nb_pp_instance, response);
    nb_var_release(response);
    nb_var_release(request);
  }
}

struct NB_Response* nb_run_message_loop_for_response(
    struct NB_Queue* message_queue,
    int id,
    int cb_id) {
  while (1) {
    struct PP_Var request_var = nb_queue_dequeue(message_queue);
    struct PP_Var response_var = PP_MakeUndefined();
    struct NB_Response* response = NULL;

    /* Try to parse the "request" as a callback response. */
    response = nb_response_parse(request_var);
    if (response) {
      int response_id = nb_response_id(response);
      int response_cb_id = nb_response_cb_id(response);
      if (response_id == id && response_cb_id == cb_id) {
        nb_var_release(request_var);
        /* Pass ownership of the response to the caller. */
        return response;
      } else {
        NB_VERROR(
            "Warning: unexpected callback response: id: %d cbId: %d. "
            "Expected id: %d cbId: %d",
            response_id, response_cb_id, id, cb_id);
      }
    } else {
      /* This should be a normal request. Run it. */
      nb_request_run(message_queue, request_var, &response_var);
      g_nb_ppb_messaging->PostMessage(g_nb_pp_instance, response_var);
    }

    nb_var_release(response_var);
    nb_var_release(request_var);
  }
}

NB_Bool nb_request_run(struct NB_Queue* message_queue,
                       struct PP_Var request_var,
                       struct PP_Var* out_response_var) {
  NB_Bool result = NB_FALSE;
  struct NB_Request* request = NULL;
  struct NB_Response* response = NULL;
  int failed_command_idx = -1;

  request = nb_request_parse(request_var);
  if (request == NULL) {
    NB_ERROR("nb_request_parse() failed.");
    goto cleanup;
  }

  response = nb_response_create(nb_request_id(request));
  if (response == NULL) {
    goto cleanup;
  }

  if (!nb_request_set_handles(request)) {
    NB_ERROR("nb_request_set_handles() failed.");
    goto cleanup;
  }

  result = NB_TRUE;

  if (!nb_request_run_commands(message_queue, request, &failed_command_idx)) {
    NB_ERROR("nb_request_run_commands() failed.");
    result = NB_FALSE;
  }

  if (!nb_request_get_handles(request, response)) {
    NB_ERROR("nb_request_get_handles() failed.");
    result = NB_FALSE;
  }

  nb_request_destroy_handles(request);

cleanup:
  if (request != NULL) {
    nb_request_destroy(request);
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

NB_Bool nb_request_set_handles(struct NB_Request* request) {
  NB_Bool result = NB_FALSE;
  int sethandles_count = nb_request_sethandles_count(request);
  int i;
  struct PP_Var value = PP_MakeUndefined();

  for (i = 0; i < sethandles_count; ++i) {
    NB_Handle handle;
    nb_request_sethandle(request, i, &handle, &value);

    switch (value.type) {
      case PP_VARTYPE_INT32:
        if (!nb_handle_register_int32(handle, value.value.as_int)) {
          NB_VERROR("nb_handle_register_int32(%d, %d) failed, i=%d.", handle,
                    value.value.as_int, i);
          goto cleanup;
        }
        break;

      case PP_VARTYPE_DOUBLE:
        if (!nb_handle_register_double(handle, value.value.as_double)) {
          NB_VERROR("nb_handle_register_double(%d, %g) failed, i=%d.", handle,
                    value.value.as_double, i);
          goto cleanup;
        }
        break;

      case PP_VARTYPE_STRING: {
        if (!nb_handle_register_var(handle, value)) {
          NB_VERROR("nb_handle_register_var(%d, %s) failed, i=%d.", handle,
                    nb_var_type_to_string(value.type), i);
          goto cleanup;
        }
        break;
      }

      case PP_VARTYPE_ARRAY: {
        const char* tag;
        uint32_t tag_length;
        uint32_t array_length;

        if (!nb_var_tagged_array(value, &tag, &tag_length, &array_length)) {
          goto cleanup;
        }

        if (strncmp(tag, "long", tag_length) == 0) {
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
        } else if (strncmp(tag, "function", tag_length) == 0) {
          NB_FuncId id;
          if (!nb_var_func_id(value, &id)) {
            NB_VERROR("nb_var_func_id(%d, %s) failed, i=%d.",
                      handle,
                      nb_var_type_to_string(value.type),
                      i);
            goto cleanup;
          }

          if (!nb_handle_register_func_id(handle, id)) {
            NB_VERROR("nb_handle_register_int64(%d, %d) failed, i=%d.",
                      handle,
                      id,
                      i);
            goto cleanup;
          }
        }
        break;
      }

      case PP_VARTYPE_NULL:
        if (!nb_handle_register_voidp(handle, NULL)) {
          NB_VERROR("nb_handle_register_voidp(%d, NULL) failed, i=%d.", handle,
                    i);
          goto cleanup;
        }
        break;

      default:
        /* This shouldn't happen; we've already validated the request before
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

NB_Bool nb_request_run_commands(struct NB_Queue* message_queue,
                                struct NB_Request* request,
                                int* out_failed_command_idx) {
  int commands_count = nb_request_commands_count(request);
  int i;

  for (i = 0; i < commands_count; ++i) {
    if (!nb_request_command_run(message_queue, request, i)) {
      *out_failed_command_idx = i;
      NB_VERROR("nb_request_command_run(%d) failed.", i);
      return NB_FALSE;
    }
  }

  return NB_TRUE;
}

NB_Bool nb_request_get_handles(struct NB_Request* request,
                               struct NB_Response* response) {
  NB_Bool result = NB_TRUE;
  int gethandles_count = nb_request_gethandles_count(request);
  int i;

  for (i = 0; i < gethandles_count; ++i) {
    NB_Handle handle = nb_request_gethandle(request, i);
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

void nb_request_destroy_handles(struct NB_Request* request) {
  int destroyhandles_count = nb_request_destroyhandles_count(request);
  NB_Handle* handles = alloca(destroyhandles_count * sizeof(NB_Handle));
  int i;

  for (i = 0; i < destroyhandles_count; ++i) {
    handles[i] = nb_request_destroyhandle(request, i);
  }

  nb_handle_destroy_many(handles, destroyhandles_count);
}
