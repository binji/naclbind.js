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
  Message* message = (Message*)malloc(sizeof(Message));
  message->var = var;
  AddRefVar(&message->var);

  struct PP_Var id_var = GetDictVar(&var, "id");
  if (!GetVarInt32(&id_var, &message->id)) {
    goto fail;
  }

  message->commands = GetDictVar(&var, "msgs");
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

  command = (Command*)malloc(sizeof(Command));
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

  command->args = GetDictVar(&var, "args");
  if (command->args.type != PP_VARTYPE_ARRAY) {
    ERROR("args is not of type array.");
    goto fail;
  }

  command->arg_is_handle = GetDictVar(&var, "argIsHandle");
  if (command->arg_is_handle.type != PP_VARTYPE_ARRAY) {
    ERROR("arg_is_handle is not of type array.");
    goto fail;
  }

  if (GetArrayVarLength(&command->args) !=
      GetArrayVarLength(&command->arg_is_handle)) {
    ERROR("arg.length != arg_is_handle.length");
    goto fail;
  }

  ret_handle_var = GetDictVar(&var, "ret");
  if (!GetVarInt32(&ret_handle_var, &command->ret_handle)) {
    ERROR("ret is not of type int.");
    goto fail;
  }

  return command;

fail:
  printf("Command failed...\n");
  if (command) {
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
  free((void*)command->command);
  ReleaseVar(&command->var);
  free(command);
}

int32_t GetCommandArgCount(Command* command) {
  return GetArrayVarLength(&command->args);
}

bool GetCommandArg(Command* command, int32_t index,
                   struct PP_Var* out_var, bool* out_is_handle) {
  assert(index < GetCommandArgCount(command));

  struct PP_Var is_handle_var = GetArrayVar(&command->arg_is_handle, index);
  if (is_handle_var.type != PP_VARTYPE_BOOL) {
    return FALSE;
  }

  *out_is_handle = is_handle_var.value.as_bool;
  *out_var = GetArrayVar(&command->args, index);
  return TRUE;
}
