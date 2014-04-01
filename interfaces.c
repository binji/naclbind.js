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

#include "interfaces.h"

#include <stdlib.h>

PP_Instance g_pp_instance = 0;
struct PPB_Var_1_1* g_ppb_var = NULL;
struct PPB_VarArray_1_0* g_ppb_var_array = NULL;
struct PPB_VarArrayBuffer_1_0* g_ppb_var_array_buffer = NULL;
struct PPB_VarDictionary_1_0* g_ppb_var_dictionary = NULL;
struct PPB_Messaging_1_0* g_ppb_messaging = NULL;

void InitInterfaces(PP_Instance instance, PPB_GetInterface get_interface) {
  g_pp_instance = instance;
  g_ppb_var = (struct PPB_Var_1_1*)get_interface(PPB_VAR_INTERFACE_1_1);
  g_ppb_var_array = (struct PPB_VarArray_1_0*)get_interface(
      PPB_VAR_ARRAY_INTERFACE_1_0);
  g_ppb_var_array_buffer = (struct PPB_VarArrayBuffer_1_0*)get_interface(
      PPB_VAR_ARRAY_BUFFER_INTERFACE_1_0);
  g_ppb_var_dictionary = (struct PPB_VarDictionary_1_0*)get_interface(
      PPB_VAR_DICTIONARY_INTERFACE_1_0);
  g_ppb_messaging = (struct PPB_Messaging_1_0*)get_interface(
      PPB_MESSAGING_INTERFACE_1_0);
}
