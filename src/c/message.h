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

#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <stdint.h>

#include <ppapi/c/pp_var.h>

#include "bool.h"
#include "handle.h"
#include "type.h"

typedef struct {
  struct PP_Var var;
  bool is_handle;
  char* string;
} Arg;

typedef struct {
  const char* command;
  Type type;
  Arg* args;
  uint32_t num_args;
  int32_t ret_handle;
} Command;

typedef struct {
  int32_t id;
  Command* commands;
  uint32_t num_commands;
  Handle* ret_handles;
  uint32_t num_ret_handles;
} Message;

Message* CreateMessage(struct PP_Var);
void DestroyMessage(Message*);
int32_t GetMessageCommandCount(Message*);
Command* GetMessageCommand(Message*, int32_t index);
int32_t GetMessageRetHandleCount(Message*);
bool GetMessageRetHandle(Message*, int32_t index, Handle* out_handle);

int32_t GetCommandArgCount(Command*);
bool GetCommandArg(Command*, int32_t index, Arg** out_arg);

bool GetArgVoidp(Command* command, int32_t index, void** out_value);
bool GetArgCharp(Command* command, int32_t index, char** out_value);
bool GetArgInt32(Command* command, int32_t index, int32_t* out_value);
bool GetArgUint32(Command* command, int32_t index, uint32_t* out_value);
bool GetArgInt64(Command* command, int32_t index, int64_t* out_value);
bool GetArgUint64(Command* command, int32_t index, uint64_t* out_value);
bool GetArgFloat32(Command* command, int32_t index, float* out_value);
bool GetArgFloat64(Command* command, int32_t index, double* out_value);
bool GetArgVar(Command* command, int32_t index, struct PP_Var* out_value);

#define CMD_VERROR(fmt, ...) \
  VERROR("%s: " fmt, command->command, __VA_ARGS__)

#define CMD_ERROR(msg) \
  VERROR("%s: " msg, command->command)

#endif  // MESSAGE_H_
