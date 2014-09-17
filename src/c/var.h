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

#ifndef VAR_H_
#define VAR_H_

#include <stdint.h>
#include <ppapi/c/pp_var.h>

#include "bool.h"

#ifdef __cplusplus
extern "C" {
#endif

void nb_var_addref(struct PP_Var);
void nb_var_release(struct PP_Var);

struct PP_Var nb_var_string_create(const char*, uint32_t len);

struct PP_Var nb_var_array_create(void);
uint32_t nb_var_array_length(struct PP_Var);
struct PP_Var nb_var_array_get(struct PP_Var, uint32_t index);
bool nb_var_array_set(struct PP_Var, uint32_t index, struct PP_Var);

struct PP_Var nb_var_dict_create(void);
struct PP_Var nb_var_dict_get(struct PP_Var, const char* key);
struct PP_Var nb_var_dict_get_var(struct PP_Var, struct PP_Var key);
bool nb_var_dict_set(struct PP_Var, const char* key, struct PP_Var);
bool nb_var_dict_set_var(struct PP_Var, struct PP_Var key, struct PP_Var);
struct PP_Var nb_var_dict_get_keys(struct PP_Var);
bool nb_var_dict_has_key(struct PP_Var, const char* key);

bool nb_var_int8(struct PP_Var, int8_t*);
bool nb_var_uint8(struct PP_Var, uint8_t*);
bool nb_var_int16(struct PP_Var, int16_t*);
bool nb_var_uint16(struct PP_Var, uint16_t*);
bool nb_var_int32(struct PP_Var, int32_t*);
bool nb_var_uint32(struct PP_Var, uint32_t*);
bool nb_var_int64(struct PP_Var, int64_t*);
bool nb_var_uint64(struct PP_Var, uint64_t*);
bool nb_var_float(struct PP_Var, float*);
bool nb_var_double(struct PP_Var, double*);
bool nb_var_string(struct PP_Var, const char**, uint32_t* out_length);

#ifdef __cplusplus
}
#endif

#endif // VAR_H_
