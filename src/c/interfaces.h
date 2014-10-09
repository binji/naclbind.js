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

#ifndef NB_INTERFACES_H_
#define NB_INTERFACES_H_

#include <ppapi/c/pp_instance.h>
#include <ppapi/c/ppb.h>
#include <ppapi/c/ppb_messaging.h>
#include <ppapi/c/ppb_var.h>
#include <ppapi/c/ppb_var_array.h>
#include <ppapi/c/ppb_var_array_buffer.h>
#include <ppapi/c/ppb_var_dictionary.h>

#ifdef __cplusplus
extern "C" {
#endif

extern PP_Instance g_nb_pp_instance;
extern struct PPB_Var_1_1* g_nb_ppb_var;
extern struct PPB_VarArray_1_0* g_nb_ppb_var_array;
extern struct PPB_VarArrayBuffer_1_0* g_nb_ppb_var_array_buffer;
extern struct PPB_VarDictionary_1_0* g_nb_ppb_var_dictionary;
extern struct PPB_Messaging_1_0* g_nb_ppb_messaging;

void nb_interfaces_init(PP_Instance, PPB_GetInterface);

#ifdef __cplusplus
}
#endif

#endif /* NB_INTERFACES_H_ */
