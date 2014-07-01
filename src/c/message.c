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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "var.h"

static void DestroyMessageCommands(Message* message);
static bool InitCommand(Command* command, struct PP_Var var);
static void DestroyCommand(Command* command);

Message* CreateMessage(struct PP_Var var) {
  Message* message = calloc(1, sizeof(Message));
  if (message == NULL) {
    ERROR("Failed to calloc Message.");
    goto fail;
  }

  struct PP_Var id_var = GetDictVar(&var, "id");
  if (!GetVarInt32(&id_var, &message->id)) {
    goto fail;
  }

  struct PP_Var commands_var = GetDictVar(&var, "commands");
  if (commands_var.type != PP_VARTYPE_ARRAY) {
    ERROR("commands is not of array type.");
    goto fail;
  }

  message->num_commands = GetArrayVarLength(&commands_var);
  message->commands = calloc(message->num_commands, sizeof(Command));
  if (message->commands == NULL) {
    ERROR("Failed to calloc Command array.");
    goto fail;
  }

  for (uint32_t i = 0; i < message->num_commands; ++i) {
    struct PP_Var command_var = GetArrayVar(&commands_var, i);
    if (!InitCommand(&message->commands[i], command_var)) {
      goto fail;
    }
  }

  struct PP_Var ret_handles = GetDictVar(&var, "handles");
  if (ret_handles.type != PP_VARTYPE_ARRAY) {
    ERROR("handles is not of array type.");
    goto fail;
  }

  message->num_ret_handles = GetArrayVarLength(&ret_handles);
  message->ret_handles = calloc(message->num_ret_handles, sizeof(Handle));
  if (message->ret_handles == NULL) {
    ERROR("Failed to calloc return handle array.");
    goto fail;
  }

  for (uint32_t i = 0; i < message->num_ret_handles; ++i) {
    struct PP_Var handle_var = GetArrayVar(&ret_handles, i);
    Handle handle;
    if (!GetVarInt32(&handle_var, &handle)) {
      VERROR("return handle %d is not integer type.", i);
      goto fail;
    }

    message->ret_handles[i] = handle;
  }

  return message;

fail:
  DestroyMessage(message);
  return NULL;
}

void DestroyMessage(Message* message) {
  assert(message != NULL);
  DestroyMessageCommands(message);
  free(message->ret_handles);
  free(message->commands);
  free(message);
}

void DestroyMessageCommands(Message* message) {
  for (uint32_t i = 0; i < message->num_commands; ++i) {
    DestroyCommand(&message->commands[i]);
  }
}

int32_t GetMessageCommandCount(Message* message) {
  return message->num_commands;
}

Command* GetMessageCommand(Message* message, int32_t index) {
  assert(index < GetMessageCommandCount(message));

  return &message->commands[index];
}

bool InitCommand(Command* command, struct PP_Var var) {
  struct PP_Var cmd_var;
  struct PP_Var type_var;
  struct PP_Var ret_handle_var;
  uint32_t cmd_length;
  const char* cmd;

  if (var.type != PP_VARTYPE_DICTIONARY) {
    ERROR("command is not a dictionary.");
    goto fail;
  }

  cmd_var = GetDictVar(&var, "cmd");
  if (cmd_var.type != PP_VARTYPE_STRING) {
    ERROR("cmd is not of type string.");
    goto fail;
  }

  if (!GetVarString(&cmd_var, &cmd, &cmd_length)) {
    ERROR("Failed to get string value of cmd.");
    goto fail;
  }
  command->command = strndup(cmd, cmd_length);

  type_var = GetDictVar(&var, "type");
  int32_t type_int;
  if (!GetVarInt32(&type_var, &type_int)) {
    ERROR("type is not of type int.");
    goto fail;
  }
  command->type = (Type)type_int;

  struct PP_Var args = GetDictVar(&var, "args");
  if (args.type != PP_VARTYPE_ARRAY) {
    ERROR("args is not of type array.");
    goto fail;
  }

  struct PP_Var arg_is_handle = GetDictVar(&var, "argIsHandle");
  if (arg_is_handle.type != PP_VARTYPE_ARRAY) {
    ERROR("arg_is_handle is not of type array.");
    goto fail;
  }

  if (GetArrayVarLength(&args) != GetArrayVarLength(&arg_is_handle)) {
    ERROR("arg.length != arg_is_handle.length");
    goto fail;
  }

  command->num_args = GetArrayVarLength(&args);
  command->args = calloc(command->num_args, sizeof(Arg));
  if (command->args == NULL) {
    ERROR("Failed to calloc Args.");
    goto fail;
  }

  for (uint32_t arg_ix = 0; arg_ix < command->num_args; ++arg_ix) {
    Arg* arg = &command->args[arg_ix];
    arg->var = GetArrayVar(&args, arg_ix);

    struct PP_Var is_handle_var = GetArrayVar(&arg_is_handle, arg_ix);
    if (is_handle_var.type != PP_VARTYPE_BOOL) {
      VERROR("arg_is_handle element %d has non-bool type: %d.",
             arg_ix, is_handle_var.type);
      goto fail;
    }

    arg->is_handle = is_handle_var.value.as_bool;
  }

  ret_handle_var = GetDictVar(&var, "ret");
  if (!GetVarInt32(&ret_handle_var, &command->ret_handle)) {
    ERROR("ret is not of type int.");
    goto fail;
  }

  return TRUE;

fail:
  fprintf(stderr, "Command failed...\n");
  if (command) {
    free(command->args);
    free((void*)command->command);
  }
  free(command);
  return FALSE;
}

int32_t GetMessageRetHandleCount(Message* message) {
  return message->num_ret_handles;
}

bool GetMessageRetHandle(Message* message, int32_t index, Handle* out_handle) {
  assert(index < GetMessageRetHandleCount(message));

  *out_handle = message->ret_handles[index];
  return TRUE;
}

void DestroyCommand(Command* command) {
  assert(command != NULL);
  for (uint32_t i = 0; i < command->num_args; ++i) {
    free(command->args[i].string);
  }
  free((void*)command->command);
}

int32_t GetCommandArgCount(Command* command) {
  return command->num_args;
}

bool GetCommandArg(Command* command, int32_t index, Arg** out_arg) {
  assert(index < command->num_args);

  *out_arg = &command->args[index];
  return TRUE;
}

bool GetArgVoidp(Command* command, int32_t index, void** out_value) {
  Arg* arg;
  if (!GetCommandArg(command, index, &arg)) {
    CMD_VERROR("Can't get arg %d", index);
    return FALSE;
  }
  if (arg->var.type == PP_VARTYPE_NULL) {
    *out_value = NULL;
    return TRUE;
  }
  if (!arg->is_handle) {
    CMD_VERROR("Expected arg %d to be handle", index);
    return FALSE;
  }
  int32_t arg_handle_int;
  if (!GetVarInt32(&arg->var, &arg_handle_int)) {
    CMD_VERROR("Expected handle arg %d to be int32_t", index);
    return FALSE;
  }

  Handle handle = arg_handle_int;
  if (!GetHandleVoidp(handle, out_value)) {
    CMD_VERROR("Expected arg %d handle's value to be void*", index);
    return FALSE;
  }

  return TRUE;
}

bool GetArgCharp(Command* command, int32_t index, char** out_value) {
  Arg* arg;
  if (!GetCommandArg(command, index, &arg)) {
    CMD_VERROR("Can't get arg %d", index);
    return FALSE;
  }
  if (arg->var.type == PP_VARTYPE_NULL) {
    *out_value = NULL;
    return TRUE;
  }
  if (arg->is_handle) {
    int32_t arg_handle_int;
    if (!GetVarInt32(&arg->var, &arg_handle_int)) {
      CMD_VERROR("Expected handle arg %d to be int32_t", index);
      return FALSE;
    }

    Handle handle = arg_handle_int;
    if (!GetHandleCharp(handle, out_value)) {
      CMD_VERROR("Expected arg %d handle's value to be char*", index);
      return FALSE;
    }
  } else {
    if (arg->var.type != PP_VARTYPE_STRING) {
      CMD_VERROR("Expected arg %d to be char*", index);
      return FALSE;
    }

    if (!arg->string) {
      const char* str;
      uint32_t len;
      if (!GetVarString(&arg->var, &str, &len)) {
        ERROR("Failed to get string value of arg.");
        return FALSE;
      }
      arg->string = strndup(str, len);
    }

    *out_value = arg->string;
  }

  return TRUE;
}

bool GetArgInt32(Command* command, int32_t index, int32_t* out_value) {
  Arg* arg;
  if (!GetCommandArg(command, index, &arg)) {
    CMD_VERROR("Can't get arg %d", index);
    return FALSE;
  }
  if (arg->is_handle) {
    int32_t arg_handle_int;
    if (!GetVarInt32(&arg->var, &arg_handle_int)) {
      CMD_VERROR("Expected handle arg %d to be int32_t", index);
      return FALSE;
    }

    Handle handle = arg_handle_int;
    if (!GetHandleInt32(handle, out_value)) {
      CMD_VERROR("Expected arg %d handle's value to be int32_t", index);
      return FALSE;
    }
  } else {
    if (!GetVarInt32(&arg->var, out_value)) {
      CMD_VERROR("Expected arg %d to be int32_t", index);
      return FALSE;
    }
  }

  return TRUE;
}

bool GetArgUint32(Command* command, int32_t index, uint32_t* out_value) {
  Arg* arg;
  if (!GetCommandArg(command, index, &arg)) {
    CMD_VERROR("Can't get arg %d", index);
    return FALSE;
  }
  if (arg->is_handle) {
    int32_t arg_handle_int;
    if (!GetVarInt32(&arg->var, &arg_handle_int)) {
      CMD_VERROR("Expected handle arg %d to be int32_t", index);
      return FALSE;
    }

    Handle handle = arg_handle_int;
    if (!GetHandleUint32(handle, out_value)) {
      CMD_VERROR("Expected arg %d handle's value to be uint32_t", index);
      return FALSE;
    }
  } else {
    if (!GetVarUint32(&arg->var, out_value)) {
      CMD_VERROR("Expected arg %d to be uint32_t", index);
      return FALSE;
    }
  }

  return TRUE;
}

bool GetArgInt64(Command* command, int32_t index, int64_t* out_value) {
  CMD_ERROR("GetArgInt64 not implemented.");
  // TODO(binji): implement
  return FALSE;
}

bool GetArgUint64(Command* command, int32_t index, uint64_t* out_value) {
  CMD_ERROR("GetArgUint64 not implemented.");
  // TODO(binji): implement
  return FALSE;
}

bool GetArgFloat32(Command* command, int32_t index, float* out_value) {
  CMD_ERROR("GetArgFloat32 not implemented.");
  // TODO(binji): implement
  return FALSE;
}

bool GetArgFloat64(Command* command, int32_t index, double* out_value) {
  CMD_ERROR("GetArgFloat64 not implemented.");
  // TODO(binji): implement
  return FALSE;
}

bool GetArgVar(Command* command, int32_t index, struct PP_Var* out_value) {
  Arg* arg;
  if (!GetCommandArg(command, index, &arg)) {
    CMD_VERROR("Can't get arg %d", index);
    return FALSE;
  }
  if (arg->is_handle) {
    int32_t arg_handle_int;
    if (!GetVarInt32(&arg->var, &arg_handle_int)) {
      CMD_VERROR("Expected handle arg %d to be int32_t", index);
      return FALSE;
    }

    Handle handle = arg_handle_int;
    if (!GetHandleVar(handle, out_value)) {
      CMD_VERROR("Expected arg %d handle's value to be uint32_t", index);
      return FALSE;
    }
  } else {
    *out_value = arg->var;
  }

  return TRUE;
}

