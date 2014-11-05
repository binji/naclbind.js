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

#ifndef RESPONSE_H_
#define RESPONSE_H_

#include <ppapi/c/pp_var.h>

#ifndef NB_ONE_FILE
#include "bool.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct NB_Response;

struct NB_Response* nb_response_create(int id);
void nb_response_destroy(struct NB_Response*);
NB_Bool nb_response_set_value(struct NB_Response*, int i, struct PP_Var value);
struct PP_Var nb_response_get_var(struct NB_Response*);
NB_Bool nb_response_set_error(struct NB_Response*, int failed_command_idx);

#ifdef __cplusplus
}
#endif

#endif /* RESPONSE_H_ */
