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

#include "handle.h"
#include "type.h"

typedef struct {
  struct PP_Var var;
  int32_t id;
  struct PP_Var commands;
  struct PP_Var ret_handles;
} Message;

typedef struct {
  struct PP_Var var;
  const char* command;
  Type type;
  struct PP_Var args;
  struct PP_Var arg_is_handle;
  int32_t ret_handle;
} Command;

Message* CreateMessage(struct PP_Var);
void DestroyMessage(Message*);
int32_t GetMessageCommandCount(Message*);
Command* GetMessageCommand(Message*, int32_t index);
int32_t GetMessageRetHandleCount(Message*);
bool GetMessageRetHandle(Message*, int32_t index,
                         Handle* out_handle, Type* out_type);

void DestroyCommand(Command*);
int32_t GetCommandArgCount(Command*);
bool GetCommandArg(Command*, int32_t index,
                   struct PP_Var* out_var, bool* out_is_handle);

#endif  // MESSAGE_H_
