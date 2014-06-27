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
#include "interfaces.h"
#include "var.h"

Message* CreateMessage(struct PP_Var var) {
  Message* message = calloc(1, sizeof(Message));
  message->var = var;
  AddRefVar(&message->var);

  struct PP_Var id_var = GetDictVar(&var, "id");
  if (!GetVarInt32(&id_var, &message->id)) {
    goto fail;
  }

  message->commands = GetDictVar(&var, "commands");
  if (message->commands.type != PP_VARTYPE_ARRAY) {
    goto fail;
  }

  message->ret_handles = GetDictVar(&var, "handles");
  if (message->ret_handles.type != PP_VARTYPE_ARRAY) {
    goto fail;
  }

  return message;

fail:
  free(message);
  return NULL;
}

void DestroyMessage(Message* message) {
  assert(message != NULL);

  ReleaseVar(&message->var);
  free(message);
}

int32_t GetMessageCommandCount(Message* message) {
  return GetArrayVarLength(&message->commands);
}

Command* GetMessageCommand(Message* message, int32_t index) {
  assert(index < GetMessageCommandCount(message));

  Command* command;
  struct PP_Var var;
  struct PP_Var cmd_var;
  struct PP_Var type_var;
  struct PP_Var ret_handle_var;
  uint32_t cmd_length;
  const char* cmd;

  var = GetArrayVar(&message->commands, index);
  if (var.type != PP_VARTYPE_DICTIONARY) {
    VERROR("command %d is not a dictionary.", index);
    return NULL;
  }

  command = calloc(1, sizeof(Command));
  if (command == NULL) {
    ERROR("Failed to calloc Command.");
    goto fail;
  }

  command->var = var;

  AddRefVar(&command->var);

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

  return command;

fail:
  fprintf(stderr, "Command failed...\n");
  if (command) {
    free(command->args);
    free((void*)command->command);
    ReleaseVar(&command->var);
  }
  free(command);
  return NULL;
}

int32_t GetMessageRetHandleCount(Message* message) {
  return GetArrayVarLength(&message->ret_handles);
}

bool GetMessageRetHandle(Message* message, int32_t index, Handle* out_handle) {
  assert(index < GetMessageRetHandleCount(message));

  struct PP_Var handle_var = GetArrayVar(&message->ret_handles, index);
  return GetVarInt32(&handle_var, out_handle);
}

void DestroyCommand(Command* command) {
  assert(command != NULL);
  for (uint32_t i = 0; i < command->num_args; ++i) {
    free(command->args[i].string);
  }
  free((void*)command->command);
  ReleaseVar(&command->var);
  free(command);
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
      uint32_t len;
      const char* str = g_ppb_var->VarToUtf8(arg->var, &len);
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

