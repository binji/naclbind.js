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

#include "message.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ppapi/c/pp_var.h>

#include "error.h"
#include "var.h"

static bool message_is_valid(struct Message* message);
static bool expect_key(struct PP_Var var, const char* key,
                       struct PP_Var* out_value);
static bool optional_key(struct PP_Var var, const char* key,
                         struct PP_Var* out_value);
static bool id_is_valid(struct PP_Var var);
static bool get_is_valid(struct PP_Var var);
static bool set_is_valid(struct PP_Var var);
static bool destroy_is_valid(struct PP_Var var);
static bool commands_is_valid(struct PP_Var var);
static bool command_is_valid(struct PP_Var var);

struct Message {
  struct PP_Var var;
};

static bool string_to_long(const char* s, uint32_t len, long* out_value) {
  enum { kBufferSize = 32 };
  char buffer[kBufferSize + 1];
  char* endptr;

  len = (len < kBufferSize) ? len : kBufferSize;
  memcpy(&buffer[0], s, len);
  buffer[len] = 0;

  errno = 0;
  *out_value = strtol(buffer, &endptr, 10);
  return (errno == 0 && endptr == buffer + len) ? TRUE : FALSE;
}

static bool var_string_to_long(struct PP_Var var, long* out_value) {
  const char* str;
  uint32_t len;

  if (!nb_var_check_type_with_error(var, PP_VARTYPE_STRING)) {
    return FALSE;
  }

  if (!nb_var_string(var, &str, &len)) {
    return FALSE;
  }

  if (!string_to_long(str, len, out_value)) {
    VERROR("Expected string to be int. Got \"%.*s\".", len, str);
    return FALSE;
  }

  return TRUE;
}

struct Message* nb_message_create(struct PP_Var var) {
  struct Message* message = calloc(1, sizeof(struct Message));
  message->var = var;
  nb_var_addref(var);

  if (!message_is_valid(message)) {
    nb_message_destroy(message);
    return NULL;
  }

  return message;
}

void nb_message_destroy(struct Message* message) {
  if (message == NULL) {
    return;
  }

  nb_var_release(message->var);
  free(message);
}

bool message_is_valid(struct Message* message) {
  struct PP_Var var = message->var;
  return nb_var_check_type_with_error(var, PP_VARTYPE_DICTIONARY) &&
         id_is_valid(var) &&
         get_is_valid(var) &&
         set_is_valid(var) &&
         destroy_is_valid(var) &&
         commands_is_valid(var);
}

bool expect_key(struct PP_Var var, const char* key, struct PP_Var* out_value) {
  bool result = optional_key(var, key, out_value);
  if (!result) {
    VERROR("Expected message to have key: %s", key);
  }

  return result;
}

bool optional_key(struct PP_Var var, const char* key,
                  struct PP_Var* out_value) {
  if (!nb_var_dict_has_key(var, key)) {
    return FALSE;
  }

  *out_value = nb_var_dict_get(var, key);
  return TRUE;
}

bool id_is_valid(struct PP_Var var) {
  bool result = FALSE;
  struct PP_Var id = PP_MakeUndefined();

  if (!expect_key(var, "id", &id)) {
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(id, PP_VARTYPE_INT32)) {
    goto cleanup;
  }

  if (id.value.as_int <= 0) {
    VERROR("Expected message id to be > 0. Got %d", id.value.as_int);
    goto cleanup;
  }

  result = TRUE;
cleanup:
  nb_var_release(id);
  return result;
}

bool get_is_valid(struct PP_Var var) {
  bool result = FALSE;
  struct PP_Var gethandles = PP_MakeUndefined();
  uint32_t i, len;

  if (!optional_key(var, "get", &gethandles)) {
    result = TRUE;
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(gethandles, PP_VARTYPE_ARRAY)) {
    goto cleanup;
  }

  len = nb_var_array_length(gethandles);
  for (i = 0; i < len; ++i) {
    struct PP_Var handle = nb_var_array_get(gethandles, i);
    if (!nb_var_check_type_with_error(handle, PP_VARTYPE_INT32)) {
      nb_var_release(handle);
      goto cleanup;
    }

    nb_var_release(handle);
  }

  result = TRUE;
cleanup:
  nb_var_release(gethandles);
  return result;
}

bool set_is_valid(struct PP_Var var) {
  bool result = FALSE;
  struct PP_Var sethandles = PP_MakeUndefined();
  struct PP_Var keys = PP_MakeUndefined();
  uint32_t i, len;

  if (!optional_key(var, "set", &sethandles)) {
    result = TRUE;
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(sethandles, PP_VARTYPE_DICTIONARY)) {
    goto cleanup;
  }

  keys = nb_var_dict_get_keys(sethandles);
  len = nb_var_array_length(keys);
  for (i = 0; i < len; ++i) {
    struct PP_Var key = nb_var_array_get(keys, i);
    long key_long;
    struct PP_Var value;

    if (!var_string_to_long(key, &key_long)) {
      nb_var_release(key);
      goto cleanup;
    }

    value = nb_var_dict_get_var(sethandles, key);
    // TODO(binji): for now, only support int/double.
    switch (value.type) {
      case PP_VARTYPE_INT32:
      case PP_VARTYPE_DOUBLE:
        break;

      default:
        VERROR("Unexpected set handle value type: %s.",
               nb_var_type_to_string(value.type));
        nb_var_release(key);
        nb_var_release(value);
        goto cleanup;
    }

    nb_var_release(value);
  }

  result = TRUE;
cleanup:
  nb_var_release(keys);
  nb_var_release(sethandles);
  return result;
}

bool destroy_is_valid(struct PP_Var var) {
  bool result = FALSE;
  struct PP_Var destroyhandles;
  uint32_t i, len;

  if (!optional_key(var, "destroy", &destroyhandles)) {
    result = TRUE;
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(destroyhandles, PP_VARTYPE_ARRAY)) {
    goto cleanup;
  }

  len = nb_var_array_length(destroyhandles);
  for (i = 0; i < len; ++i) {
    struct PP_Var handle = nb_var_array_get(destroyhandles, i);
    if (!nb_var_check_type_with_error(handle, PP_VARTYPE_INT32)) {
      nb_var_release(handle);
      nb_var_release(destroyhandles);
      return FALSE;
    }

    nb_var_release(handle);
  }

  result = TRUE;
cleanup:
  nb_var_release(destroyhandles);
  return result;
}

bool commands_is_valid(struct PP_Var var) {
  bool result = FALSE;
  struct PP_Var commands = PP_MakeUndefined();
  uint32_t i, len;

  if (!optional_key(var, "commands", &commands)) {
    result = TRUE;
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(commands, PP_VARTYPE_ARRAY)) {
    goto cleanup;
  }

  len = nb_var_array_length(commands);
  for (i = 0; i < len; ++i) {
    struct PP_Var command = nb_var_array_get(commands, i);
    if (!command_is_valid(command)) {
      nb_var_release(command);
      goto cleanup;
    }

    nb_var_release(command);
  }

  result = TRUE;
cleanup:
  nb_var_release(commands);
  return result;
}

bool command_is_valid(struct PP_Var command) {
  bool result = FALSE;
  struct PP_Var id = PP_MakeUndefined();
  struct PP_Var args = PP_MakeUndefined();
  struct PP_Var ret = PP_MakeUndefined();
  uint32_t i, len;

  if (!nb_var_check_type_with_error(command, PP_VARTYPE_DICTIONARY)) {
    goto cleanup;
  }

  if (!expect_key(command, "id", &id) ||
      !expect_key(command, "args", &args)) {
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(id, PP_VARTYPE_INT32)) {
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(args, PP_VARTYPE_ARRAY)) {
    goto cleanup;
  }

  ret = nb_var_dict_get(command, "ret");
  if (ret.type != PP_VARTYPE_INT32 &&
      ret.type != PP_VARTYPE_UNDEFINED) {
    VERROR("Expected ret field to be int32 or undefined, not %s.",
           nb_var_type_to_string(ret.type));
    goto cleanup;
  }

  // Check that args is an array of ints.
  len = nb_var_array_length(args);
  for (i = 0; i < len; ++i) {
    struct PP_Var arg = nb_var_array_get(args, i);
    if (!nb_var_check_type_with_error(arg, PP_VARTYPE_INT32)) {
      nb_var_release(arg);
      goto cleanup;
    }

    nb_var_release(arg);
  }

  result = TRUE;
cleanup:
  nb_var_release(ret);
  nb_var_release(args);
  nb_var_release(id);
  return result;
}

int nb_message_sethandles_count(struct Message* message) {
}

void nb_message_sethandles(struct Message* message, int index,
                           Handle* out_handle, struct PP_Var* value) {
}

int nb_message_gethandles_count(struct Message* message) {
}

Handle nb_message_gethandle(struct Message* message, int index) {
}

int nb_message_destroyhandles_count(struct Message* message) {
}

Handle nb_message_destroyhandles(struct Message* message, int index) {
}

int nb_message_command_count(struct Message* message) {
}

int nb_message_command_function(struct Message* message, int command_idx) {
}

int nb_message_command_arg_count(struct Message* message, int command_idx) {
}

Handle nb_message_command_arg(struct Message* message, int command_idx,
                              int arg_idx) {
}

bool nb_message_command_has_ret(struct Message* message, int command_idx) {
}

Handle nb_message_command_ret(struct Message* message, int command_idx) {
}
