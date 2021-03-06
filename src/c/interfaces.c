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
#include "interfaces.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#define NB_INTERFACES                                        \
  X(var, Var, VAR, 1_1)                                      \
  X(var_array, VarArray, VAR_ARRAY, 1_0)                     \
  X(var_array_buffer, VarArrayBuffer, VAR_ARRAY_BUFFER, 1_0) \
  X(var_dictionary, VarDictionary, VAR_DICTIONARY, 1_0)      \
  X(messaging, Messaging, MESSAGING, 1_0)

PP_Instance g_nb_pp_instance = 0;
PPB_GetInterface g_nb_ppb_get_interface = NULL;
#define X(var, s, d, v) struct PPB_##s##_##v* g_nb_ppb_##var = NULL;
NB_INTERFACES
#undef X

void nb_interfaces_init(PP_Instance instance, PPB_GetInterface get_interface) {
  g_nb_pp_instance = instance;
  g_nb_ppb_get_interface = get_interface;
#define X(var, s, d, v) \
  g_nb_ppb_##var =      \
      (struct PPB_##s##_##v*)get_interface(PPB_##d##_INTERFACE_##v);
  NB_INTERFACES
#undef X
}
