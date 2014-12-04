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
#include "request.h"
#endif

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ppapi/c/pp_var.h>

#ifndef NB_ONE_FILE
#include "error.h"
#include "handle.h"
#include "var.h"
#endif

struct NB_Request;
struct NB_Command;

static void* nb_calloc_list(uint32_t len, size_t element_size);
static NB_Bool nb_expect_key(struct PP_Var var,
                             const char* key,
                             struct PP_Var* out_value);
static NB_Bool nb_optional_key(struct PP_Var var,
                               const char* key,
                               struct PP_Var* out_value);
static NB_Bool nb_parse_request(struct NB_Request* request, struct PP_Var var);
static NB_Bool nb_parse_id(struct NB_Request* request, struct PP_Var var);
static NB_Bool nb_parse_gethandles(struct NB_Request* request,
                                   struct PP_Var var);
static NB_Bool nb_parse_sethandles(struct NB_Request* request,
                                   struct PP_Var var);
static NB_Bool nb_parse_destroyhandles(struct NB_Request* request,
                                       struct PP_Var var);
static NB_Bool nb_parse_commands(struct NB_Request* request, struct PP_Var var);
static NB_Bool nb_parse_command(struct NB_Command* command, struct PP_Var var);

struct NB_Command {
  int id;
  NB_Handle* args;
  uint32_t args_count;
  NB_Handle ret;
};

struct NB_HandleVarPair {
  NB_Handle id;
  struct PP_Var var;
};

struct NB_Request {
  int id;
  NB_Handle* gethandles;
  uint32_t gethandles_count;
  struct NB_HandleVarPair* sethandles;
  uint32_t sethandles_count;
  NB_Handle* destroyhandles;
  uint32_t destroyhandles_count;
  struct NB_Command* commands;
  uint32_t commands_count;
};

static NB_Bool nb_string_to_long(const char* s, uint32_t len, long* out_value) {
  enum { kBufferSize = 32 };
  char buffer[kBufferSize + 1];
  char* endptr;

  len = (len < kBufferSize) ? len : kBufferSize;
  memcpy(&buffer[0], s, len);
  buffer[len] = 0;

  errno = 0;
  *out_value = strtol(buffer, &endptr, 10);
  return (errno == 0 && endptr == buffer + len) ? NB_TRUE : NB_FALSE;
}

static NB_Bool nb_var_string_to_long(struct PP_Var var, long* out_value) {
  const char* str;
  uint32_t len;

  if (!nb_var_check_type_with_error(var, PP_VARTYPE_STRING)) {
    return NB_FALSE;
  }

  if (!nb_var_string(var, &str, &len)) {
    return NB_FALSE;
  }

  if (!nb_string_to_long(str, len, out_value)) {
    NB_VERROR("Expected string to be int. Got \"%.*s\".", len, str);
    return NB_FALSE;
  }

  return NB_TRUE;
}

struct NB_Request* nb_request_create(struct PP_Var var) {
  struct NB_Request* request = calloc(1, sizeof(struct NB_Request));
  if (!nb_parse_request(request, var)) {
    nb_request_destroy(request);
    return NULL;
  }

  return request;
}

void nb_request_destroy(struct NB_Request* request) {
  uint32_t i;
  assert(request != NULL);

  for (i = 0; i < request->commands_count; ++i) {
    free(request->commands[i].args);
  }
  free(request->commands);
  free(request->destroyhandles);

  for (i = 0; i < request->sethandles_count; ++i) {
    nb_var_release(request->sethandles[i].var);
  }
  free(request->sethandles);
  free(request->gethandles);
  free(request);
}

void* nb_calloc_list(uint32_t len, size_t element_size) {
  return len ? calloc(len, element_size) : NULL;
}

NB_Bool nb_expect_key(struct PP_Var var,
                      const char* key,
                      struct PP_Var* out_value) {
  NB_Bool result = nb_optional_key(var, key, out_value);
  if (!result) {
    NB_VERROR("Expected request to have key: %s", key);
  }

  return result;
}

NB_Bool nb_optional_key(struct PP_Var var,
                        const char* key,
                        struct PP_Var* out_value) {
  if (!nb_var_dict_has_key(var, key)) {
    return NB_FALSE;
  }

  *out_value = nb_var_dict_get(var, key);
  return NB_TRUE;
}

NB_Bool nb_parse_request(struct NB_Request* request, struct PP_Var var) {
  return nb_var_check_type_with_error(var, PP_VARTYPE_DICTIONARY) &&
         nb_parse_id(request, var) && nb_parse_gethandles(request, var) &&
         nb_parse_sethandles(request, var) &&
         nb_parse_destroyhandles(request, var) &&
         nb_parse_commands(request, var);
}

NB_Bool nb_parse_id(struct NB_Request* request, struct PP_Var var) {
  NB_Bool result = NB_FALSE;
  struct PP_Var id = PP_MakeUndefined();

  if (!nb_expect_key(var, "id", &id)) {
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(id, PP_VARTYPE_INT32)) {
    goto cleanup;
  }

  if (id.value.as_int <= 0) {
    NB_VERROR("Expected request id to be > 0. Got %d", id.value.as_int);
    goto cleanup;
  }

  request->id = id.value.as_int;
  result = NB_TRUE;
cleanup:
  nb_var_release(id);
  return result;
}

NB_Bool nb_parse_gethandles(struct NB_Request* request, struct PP_Var var) {
  NB_Bool result = NB_FALSE;
  struct PP_Var gethandles_var = PP_MakeUndefined();
  NB_Handle* gethandles = NULL;
  uint32_t i, len;

  if (!nb_optional_key(var, "get", &gethandles_var)) {
    result = NB_TRUE;
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(gethandles_var, PP_VARTYPE_ARRAY)) {
    goto cleanup;
  }

  len = nb_var_array_length(gethandles_var);
  gethandles = nb_calloc_list(len, sizeof(NB_Handle));
  for (i = 0; i < len; ++i) {
    struct PP_Var handle = nb_var_array_get(gethandles_var, i);
    if (!nb_var_check_type_with_error(handle, PP_VARTYPE_INT32)) {
      nb_var_release(handle);
      goto cleanup;
    }

    gethandles[i] = handle.value.as_int;
    nb_var_release(handle);
  }

  request->gethandles = gethandles;
  request->gethandles_count = len;

  result = NB_TRUE;
  gethandles = NULL; /* Pass ownership to the request. */
cleanup:
  free(gethandles);
  nb_var_release(gethandles_var);
  return result;
}

NB_Bool nb_parse_sethandles(struct NB_Request* request, struct PP_Var var) {
  NB_Bool result = NB_FALSE;
  struct PP_Var sethandles_var = PP_MakeUndefined();
  struct PP_Var keys = PP_MakeUndefined();
  struct NB_HandleVarPair* sethandles = NULL;
  uint32_t i, len;
  struct PP_Var value;

  if (!nb_optional_key(var, "set", &sethandles_var)) {
    result = NB_TRUE;
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(sethandles_var, PP_VARTYPE_DICTIONARY)) {
    goto cleanup;
  }

  keys = nb_var_dict_get_keys(sethandles_var);
  len = nb_var_array_length(keys);
  sethandles = nb_calloc_list(len, sizeof(struct NB_HandleVarPair));
  for (i = 0; i < len; ++i) {
    struct PP_Var key = nb_var_array_get(keys, i);
    long key_long;

    if (!nb_var_string_to_long(key, &key_long)) {
      nb_var_release(key);
      goto cleanup;
    }

    value = nb_var_dict_get_var(sethandles_var, key);
    nb_var_release(key);

    switch (value.type) {
      case PP_VARTYPE_INT32:
      case PP_VARTYPE_DOUBLE:
      case PP_VARTYPE_NULL:
      case PP_VARTYPE_STRING:
        break;

      case PP_VARTYPE_ARRAY: {
        /* For now, all arrays are longs. */
        int64_t i64_value;
        if (!nb_var_int64(value, &i64_value)) {
          NB_ERROR("Unable to parse set handle value as \"long\".");
          goto cleanup;
        }

        break;
      }

      default:
        NB_VERROR("Unexpected set handle value type: %s.",
                  nb_var_type_to_string(value.type));
        goto cleanup;
    }

    sethandles[i].id = key_long;
    /* NOTE: this passes the reference from nb_var_dict_get_var above to
       sethandles[i].var. */
    sethandles[i].var = value;
    value = PP_MakeUndefined(); /* Don't release below in cleanup */
  }

  request->sethandles = sethandles;
  request->sethandles_count = len;
  result = NB_TRUE;
  sethandles = NULL; /* Pass ownership to the request. */
cleanup:
  nb_var_release(value);
  free(sethandles);
  nb_var_release(keys);
  nb_var_release(sethandles_var);
  return result;
}

NB_Bool nb_parse_destroyhandles(struct NB_Request* request, struct PP_Var var) {
  NB_Bool result = NB_FALSE;
  struct PP_Var destroyhandles_var = PP_MakeUndefined();
  NB_Handle* destroyhandles = NULL;
  uint32_t i, len;

  if (!nb_optional_key(var, "destroy", &destroyhandles_var)) {
    result = NB_TRUE;
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(destroyhandles_var, PP_VARTYPE_ARRAY)) {
    goto cleanup;
  }

  len = nb_var_array_length(destroyhandles_var);
  destroyhandles = nb_calloc_list(len, sizeof(NB_Handle));
  for (i = 0; i < len; ++i) {
    struct PP_Var handle = nb_var_array_get(destroyhandles_var, i);
    if (!nb_var_check_type_with_error(handle, PP_VARTYPE_INT32)) {
      nb_var_release(handle);
      goto cleanup;
    }

    destroyhandles[i] = handle.value.as_int;
    nb_var_release(handle);
  }

  request->destroyhandles = destroyhandles;
  request->destroyhandles_count = len;
  result = NB_TRUE;
  destroyhandles = NULL; /* Pass ownership to the request. */
cleanup:
  free(destroyhandles);
  nb_var_release(destroyhandles_var);
  return result;
}

NB_Bool nb_parse_commands(struct NB_Request* request, struct PP_Var var) {
  NB_Bool result = NB_FALSE;
  struct PP_Var commands_var = PP_MakeUndefined();
  struct NB_Command* commands = NULL;
  uint32_t i, len;

  if (!nb_optional_key(var, "commands", &commands_var)) {
    result = NB_TRUE;
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(commands_var, PP_VARTYPE_ARRAY)) {
    goto cleanup;
  }

  len = nb_var_array_length(commands_var);
  commands = nb_calloc_list(len, sizeof(struct NB_Command));
  for (i = 0; i < len; ++i) {
    struct PP_Var command_var = nb_var_array_get(commands_var, i);
    if (!nb_parse_command(&commands[i], command_var)) {
      nb_var_release(command_var);
      goto cleanup;
    }

    nb_var_release(command_var);
  }

  request->commands = commands;
  request->commands_count = len;
  result = NB_TRUE;
  commands = NULL; /* Pass ownership to the request. */
cleanup:
  free(commands);
  nb_var_release(commands_var);
  return result;
}

NB_Bool nb_parse_command(struct NB_Command* command, struct PP_Var var) {
  NB_Bool result = NB_FALSE;
  struct PP_Var id_var = PP_MakeUndefined();
  struct PP_Var args_var = PP_MakeUndefined();
  struct PP_Var ret_var = PP_MakeUndefined();
  NB_Handle* args = NULL;
  uint32_t i, len;

  if (!nb_var_check_type_with_error(var, PP_VARTYPE_DICTIONARY)) {
    goto cleanup;
  }

  if (!nb_expect_key(var, "id", &id_var) ||
      !nb_expect_key(var, "args", &args_var)) {
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(id_var, PP_VARTYPE_INT32)) {
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(args_var, PP_VARTYPE_ARRAY)) {
    goto cleanup;
  }

  ret_var = nb_var_dict_get(var, "ret");
  if (ret_var.type != PP_VARTYPE_INT32 &&
      ret_var.type != PP_VARTYPE_UNDEFINED) {
    NB_VERROR("Expected ret field to be int32 or undefined, not %s.",
              nb_var_type_to_string(ret_var.type));
    goto cleanup;
  }

  /* Check that args_var is an array of ints. */
  len = nb_var_array_length(args_var);
  args = nb_calloc_list(len, sizeof(NB_Handle));
  for (i = 0; i < len; ++i) {
    struct PP_Var arg = nb_var_array_get(args_var, i);
    if (!nb_var_check_type_with_error(arg, PP_VARTYPE_INT32)) {
      nb_var_release(arg);
      goto cleanup;
    }

    args[i] = arg.value.as_int;
    nb_var_release(arg);
  }

  command->id = id_var.value.as_int;
  command->args = args;
  command->args_count = len;
  if (ret_var.type != PP_VARTYPE_UNDEFINED) {
    command->ret = ret_var.value.as_int;
  }
  result = NB_TRUE;
  args = NULL; /* Pass ownership to the request. */
cleanup:
  free(args);
  nb_var_release(ret_var);
  nb_var_release(args_var);
  nb_var_release(id_var);
  return result;
}

int nb_request_id(struct NB_Request* request) {
  assert(request != NULL);
  return request->id;
}

int nb_request_sethandles_count(struct NB_Request* request) {
  assert(request != NULL);
  return request->sethandles_count;
}

void nb_request_sethandle(struct NB_Request* request,
                          int index,
                          NB_Handle* out_handle,
                          struct PP_Var* out_value) {
  assert(request != NULL);
  assert(index >= 0 && index < request->sethandles_count);
  assert(out_handle != NULL);
  assert(out_value != NULL);
  *out_handle = request->sethandles[index].id;
  *out_value = request->sethandles[index].var;
  nb_var_addref(*out_value);
}

int nb_request_gethandles_count(struct NB_Request* request) {
  assert(request != NULL);
  return request->gethandles_count;
}

NB_Handle nb_request_gethandle(struct NB_Request* request, int index) {
  assert(request != NULL);
  assert(index >= 0 && index < request->gethandles_count);
  return request->gethandles[index];
}

int nb_request_destroyhandles_count(struct NB_Request* request) {
  assert(request != NULL);
  return request->destroyhandles_count;
}

NB_Handle nb_request_destroyhandle(struct NB_Request* request, int index) {
  assert(request != NULL);
  assert(index >= 0 && index < request->destroyhandles_count);
  return request->destroyhandles[index];
}

int nb_request_commands_count(struct NB_Request* request) {
  assert(request != NULL);
  return request->commands_count;
}

int nb_request_command_function(struct NB_Request* request, int command_idx) {
  assert(request != NULL);
  assert(command_idx >= 0 && command_idx < request->commands_count);
  return request->commands[command_idx].id;
}

int nb_request_command_arg_count(struct NB_Request* request, int command_idx) {
  assert(request != NULL);
  assert(command_idx >= 0 && command_idx < request->commands_count);
  return request->commands[command_idx].args_count;
}

NB_Handle nb_request_command_arg(struct NB_Request* request,
                                 int command_idx,
                                 int arg_idx) {
  assert(request != NULL);
  assert(command_idx >= 0 && command_idx < request->commands_count);
  assert(arg_idx >= 0 && arg_idx < request->commands[command_idx].args_count);
  return request->commands[command_idx].args[arg_idx];
}

NB_Bool nb_request_command_has_ret(struct NB_Request* request,
                                   int command_idx) {
  assert(request != NULL);
  assert(command_idx >= 0 && command_idx < request->commands_count);
  return request->commands[command_idx].ret != 0 ? NB_TRUE : NB_FALSE;
}

NB_Handle nb_request_command_ret(struct NB_Request* request, int command_idx) {
  assert(request != NULL);
  assert(command_idx >= 0 && command_idx < request->commands_count);
  return request->commands[command_idx].ret;
}
