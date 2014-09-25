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
#include "message.h"
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

struct Message;
struct Command;

static void* calloc_list(uint32_t len, size_t element_size);
static bool expect_key(struct PP_Var var, const char* key,
                       struct PP_Var* out_value);
static bool optional_key(struct PP_Var var, const char* key,
                         struct PP_Var* out_value);
static bool parse_message(struct Message* message, struct PP_Var var);
static bool parse_id(struct Message* message, struct PP_Var var);
static bool parse_gethandles(struct Message* message, struct PP_Var var);
static bool parse_sethandles(struct Message* message, struct PP_Var var);
static bool parse_destroyhandles(struct Message* message, struct PP_Var var);
static bool parse_commands(struct Message* message, struct PP_Var var);
static bool parse_command(struct Command* command, struct PP_Var var);

struct Command {
  int id;
  Handle* args;
  uint32_t args_count;
  Handle ret;
};

struct HandleVarPair {
  Handle id;
  struct PP_Var var;
};

struct Message {
  int id;
  Handle* gethandles;
  uint32_t gethandles_count;
  struct HandleVarPair* sethandles;
  uint32_t sethandles_count;
  Handle* destroyhandles;
  uint32_t destroyhandles_count;
  struct Command* commands;
  uint32_t commands_count;
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
  if (!parse_message(message, var)) {
    nb_message_destroy(message);
    return NULL;
  }

  return message;
}

void nb_message_destroy(struct Message* message) {
  uint32_t i;
  assert(message != NULL);

  for (i = 0; i < message->commands_count; ++i) {
    free(message->commands[i].args);
  }
  free(message->commands);
  free(message->destroyhandles);

  for (i = 0; i < message->sethandles_count; ++i) {
    nb_var_release(message->sethandles[i].var);
  }
  free(message->sethandles);
  free(message->gethandles);
  free(message);
}

void* calloc_list(uint32_t len, size_t element_size) {
  return len ? calloc(len, element_size) : NULL;
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

bool parse_message(struct Message* message, struct PP_Var var) {
  return nb_var_check_type_with_error(var, PP_VARTYPE_DICTIONARY) &&
         parse_id(message, var) &&
         parse_gethandles(message, var) &&
         parse_sethandles(message, var) &&
         parse_destroyhandles(message, var) &&
         parse_commands(message, var);
}

bool parse_id(struct Message* message, struct PP_Var var) {
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

  message->id = id.value.as_int;
  result = TRUE;
cleanup:
  nb_var_release(id);
  return result;
}

bool parse_gethandles(struct Message* message, struct PP_Var var) {
  bool result = FALSE;
  struct PP_Var gethandles_var = PP_MakeUndefined();
  Handle* gethandles = NULL;
  uint32_t i, len;

  if (!optional_key(var, "get", &gethandles_var)) {
    result = TRUE;
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(gethandles_var, PP_VARTYPE_ARRAY)) {
    goto cleanup;
  }

  len = nb_var_array_length(gethandles_var);
  gethandles = calloc_list(len, sizeof(Handle));
  for (i = 0; i < len; ++i) {
    struct PP_Var handle = nb_var_array_get(gethandles_var, i);
    if (!nb_var_check_type_with_error(handle, PP_VARTYPE_INT32)) {
      nb_var_release(handle);
      goto cleanup;
    }

    gethandles[i] = handle.value.as_int;
    nb_var_release(handle);
  }

  message->gethandles = gethandles;
  message->gethandles_count = len;

  result = TRUE;
  gethandles = NULL;  /* Pass ownership to the message. */
cleanup:
  free(gethandles);
  nb_var_release(gethandles_var);
  return result;
}

bool parse_sethandles(struct Message* message, struct PP_Var var) {
  bool result = FALSE;
  struct PP_Var sethandles_var = PP_MakeUndefined();
  struct PP_Var keys = PP_MakeUndefined();
  struct HandleVarPair* sethandles = NULL;
  uint32_t i, len;

  if (!optional_key(var, "set", &sethandles_var)) {
    result = TRUE;
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(sethandles_var, PP_VARTYPE_DICTIONARY)) {
    goto cleanup;
  }

  keys = nb_var_dict_get_keys(sethandles_var);
  len = nb_var_array_length(keys);
  sethandles = calloc_list(len, sizeof(struct HandleVarPair));
  for (i = 0; i < len; ++i) {
    struct PP_Var key = nb_var_array_get(keys, i);
    long key_long;
    struct PP_Var value;

    if (!var_string_to_long(key, &key_long)) {
      nb_var_release(key);
      goto cleanup;
    }

    value = nb_var_dict_get_var(sethandles_var, key);
    nb_var_release(key);

    /* TODO(binji): for now, only support int/double/NULL. */
    switch (value.type) {
      case PP_VARTYPE_INT32:
      case PP_VARTYPE_DOUBLE:
      case PP_VARTYPE_NULL:
        break;

      default:
        VERROR("Unexpected set handle value type: %s.",
               nb_var_type_to_string(value.type));
        nb_var_release(value);
        goto cleanup;
    }

    sethandles[i].id = key_long;
    /* NOTE: this passes the reference from nb_var_dict_get_var above to
       sethandles[i].var. */
    sethandles[i].var = value;
  }

  message->sethandles = sethandles;
  message->sethandles_count = len;
  result = TRUE;
  sethandles = NULL;  /* Pass ownership to the message. */
cleanup:
  free(sethandles);
  nb_var_release(keys);
  nb_var_release(sethandles_var);
  return result;
}

bool parse_destroyhandles(struct Message* message, struct PP_Var var) {
  bool result = FALSE;
  struct PP_Var destroyhandles_var = PP_MakeUndefined();
  Handle* destroyhandles = NULL;
  uint32_t i, len;

  if (!optional_key(var, "destroy", &destroyhandles_var)) {
    result = TRUE;
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(destroyhandles_var, PP_VARTYPE_ARRAY)) {
    goto cleanup;
  }

  len = nb_var_array_length(destroyhandles_var);
  destroyhandles = calloc_list(len, sizeof(Handle));
  for (i = 0; i < len; ++i) {
    struct PP_Var handle = nb_var_array_get(destroyhandles_var, i);
    if (!nb_var_check_type_with_error(handle, PP_VARTYPE_INT32)) {
      nb_var_release(handle);
      goto cleanup;
    }

    destroyhandles[i] = handle.value.as_int;
    nb_var_release(handle);
  }

  message->destroyhandles = destroyhandles;
  message->destroyhandles_count = len;
  result = TRUE;
  destroyhandles = NULL;  /* Pass ownership to the message. */
cleanup:
  free(destroyhandles);
  nb_var_release(destroyhandles_var);
  return result;
}

bool parse_commands(struct Message* message, struct PP_Var var) {
  bool result = FALSE;
  struct PP_Var commands_var = PP_MakeUndefined();
  struct Command* commands = NULL;
  uint32_t i, len;

  if (!optional_key(var, "commands", &commands_var)) {
    result = TRUE;
    goto cleanup;
  }

  if (!nb_var_check_type_with_error(commands_var, PP_VARTYPE_ARRAY)) {
    goto cleanup;
  }

  len = nb_var_array_length(commands_var);
  commands = calloc_list(len, sizeof(struct Command));
  for (i = 0; i < len; ++i) {
    struct PP_Var command_var = nb_var_array_get(commands_var, i);
    if (!parse_command(&commands[i], command_var)) {
      nb_var_release(command_var);
      goto cleanup;
    }

    nb_var_release(command_var);
  }

  message->commands = commands;
  message->commands_count = len;
  result = TRUE;
  commands = NULL;  /* Pass ownership to the message. */
cleanup:
  free(commands);
  nb_var_release(commands_var);
  return result;
}

bool parse_command(struct Command* command, struct PP_Var var) {
  bool result = FALSE;
  struct PP_Var id_var = PP_MakeUndefined();
  struct PP_Var args_var = PP_MakeUndefined();
  struct PP_Var ret_var = PP_MakeUndefined();
  Handle* args = NULL;
  uint32_t i, len;

  if (!nb_var_check_type_with_error(var, PP_VARTYPE_DICTIONARY)) {
    goto cleanup;
  }

  if (!expect_key(var, "id", &id_var) || !expect_key(var, "args", &args_var)) {
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
    VERROR("Expected ret field to be int32 or undefined, not %s.",
           nb_var_type_to_string(ret_var.type));
    goto cleanup;
  }

  /* Check that args_var is an array of ints. */
  len = nb_var_array_length(args_var);
  args = calloc_list(len, sizeof(Handle));
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
  result = TRUE;
  args = NULL;  /* Pass ownership to the message. */
cleanup:
  free(args);
  nb_var_release(ret_var);
  nb_var_release(args_var);
  nb_var_release(id_var);
  return result;
}

int nb_message_id(struct Message* message) {
  assert(message != NULL);
  return message->id;
}

int nb_message_sethandles_count(struct Message* message) {
  assert(message != NULL);
  return message->sethandles_count;
}

void nb_message_sethandle(struct Message* message, int index,
                          Handle* out_handle, struct PP_Var* out_value) {
  assert(message != NULL);
  assert(index >= 0 && index < message->sethandles_count);
  assert(out_handle != NULL);
  assert(out_value != NULL);
  *out_handle = message->sethandles[index].id;
  *out_value = message->sethandles[index].var;
  nb_var_addref(*out_value);
}

int nb_message_gethandles_count(struct Message* message) {
  assert(message != NULL);
  return message->gethandles_count;
}

Handle nb_message_gethandle(struct Message* message, int index) {
  assert(message != NULL);
  assert(index >= 0 && index < message->gethandles_count);
  return message->gethandles[index];
}

int nb_message_destroyhandles_count(struct Message* message) {
  assert(message != NULL);
  return message->destroyhandles_count;
}

Handle nb_message_destroyhandle(struct Message* message, int index) {
  assert(message != NULL);
  assert(index >= 0 && index < message->destroyhandles_count);
  return message->destroyhandles[index];
}

int nb_message_commands_count(struct Message* message) {
  assert(message != NULL);
  return message->commands_count;
}

int nb_message_command_function(struct Message* message, int command_idx) {
  assert(message != NULL);
  assert(command_idx >= 0 && command_idx < message->commands_count);
  return message->commands[command_idx].id;
}

int nb_message_command_arg_count(struct Message* message, int command_idx) {
  assert(message != NULL);
  assert(command_idx >= 0 && command_idx < message->commands_count);
  return message->commands[command_idx].args_count;
}

Handle nb_message_command_arg(struct Message* message, int command_idx,
                              int arg_idx) {
  assert(message != NULL);
  assert(command_idx >= 0 && command_idx < message->commands_count);
  assert(arg_idx >= 0 && arg_idx < message->commands[command_idx].args_count);
  return message->commands[command_idx].args[arg_idx];
}

bool nb_message_command_has_ret(struct Message* message, int command_idx) {
  assert(message != NULL);
  assert(command_idx >= 0 && command_idx < message->commands_count);
  return message->commands[command_idx].ret != 0 ? TRUE : FALSE;
}

Handle nb_message_command_ret(struct Message* message, int command_idx) {
  assert(message != NULL);
  assert(command_idx >= 0 && command_idx < message->commands_count);
  return message->commands[command_idx].ret;
}
