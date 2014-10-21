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
#include "type.h"
#endif

#include <stdlib.h>

static const char* s_nb_type_string[] = {
    "(invalid)",
    "int8_t",
    "uint8_t",
    "int16_t",
    "uint16_t",
    "int32_t",
    "uint32_t",
    "int64_t",
    "uint64_t",
    "float",
    "double",
    "void*",
    "void(*)(void)",
    "struct PP_Var",
};

const char* nb_type_to_string(NB_Type id) {
  if (id <= 0 || id >= NB_NUM_BUILTIN_TYPES) {
    return "<unknown>";
  }
  return s_nb_type_string[id];
}
