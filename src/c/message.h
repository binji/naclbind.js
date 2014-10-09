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

#ifndef NB_MESSAGE_H_
#define NB_MESSAGE_H_

#include <ppapi/c/pp_var.h>

#ifndef NB_ONE_FILE
#include "bool.h"
#include "handle.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct NB_Message;

struct NB_Message* nb_message_create(struct PP_Var);
void nb_message_destroy(struct NB_Message*);

int nb_message_id(struct NB_Message* NB_Message);

int nb_message_sethandles_count(struct NB_Message*);
void nb_message_sethandle(struct NB_Message*,
                          int index,
                          NB_Handle* out_handle,
                          struct PP_Var* value);

int nb_message_gethandles_count(struct NB_Message*);
NB_Handle nb_message_gethandle(struct NB_Message*, int index);

int nb_message_destroyhandles_count(struct NB_Message*);
NB_Handle nb_message_destroyhandle(struct NB_Message*, int index);

int nb_message_commands_count(struct NB_Message*);
int nb_message_command_function(struct NB_Message*, int command_idx);
int nb_message_command_arg_count(struct NB_Message*, int command_idx);
NB_Handle nb_message_command_arg(struct NB_Message*,
                                 int command_idx,
                                 int arg_idx);
NB_Bool nb_message_command_has_ret(struct NB_Message*, int command_idx);
NB_Handle nb_message_command_ret(struct NB_Message*, int command_idx);

#ifdef __cplusplus
}
#endif

#endif /* NB_MESSAGE_H_ */
