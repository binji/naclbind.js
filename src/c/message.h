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

#include <ppapi/c/pp_var.h>

#include "bool.h"
#include "handle.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Message;

struct Message* nb_message_create(struct PP_Var);
void nb_message_destroy(struct Message*);

int nb_message_id(struct Message* message);

int nb_message_sethandles_count(struct Message*);
void nb_message_sethandle(struct Message*, int index,
                          Handle* out_handle, struct PP_Var* value);

int nb_message_gethandles_count(struct Message*);
Handle nb_message_gethandle(struct Message*, int index);

int nb_message_destroyhandles_count(struct Message*);
Handle nb_message_destroyhandle(struct Message*, int index);

int nb_message_commands_count(struct Message*);
int nb_message_command_function(struct Message*, int command_idx);
int nb_message_command_arg_count(struct Message*, int command_idx);
Handle nb_message_command_arg(struct Message*, int command_idx, int arg_idx);
bool nb_message_command_has_ret(struct Message*, int command_idx);
Handle nb_message_command_ret(struct Message*, int command_idx);

#ifdef __cplusplus
}
#endif

#endif  // MESSAGE_H_
