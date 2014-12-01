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

#ifndef NB_VAR_H_
#define NB_VAR_H_

#include <stdint.h>
#include <ppapi/c/pp_var.h>

#ifndef NB_ONE_FILE
#include "bool.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void nb_var_addref(struct PP_Var);
void nb_var_release(struct PP_Var);

const char* nb_var_type_to_string(PP_VarType);
NB_Bool nb_var_check_type(struct PP_Var, PP_VarType);
NB_Bool nb_var_check_type_with_error(struct PP_Var, PP_VarType);

struct PP_Var nb_var_string_create(const char*, uint32_t len);

struct PP_Var nb_var_array_create(void);
uint32_t nb_var_array_length(struct PP_Var);
struct PP_Var nb_var_array_get(struct PP_Var, uint32_t index);
NB_Bool nb_var_array_set(struct PP_Var, uint32_t index, struct PP_Var);

struct PP_Var nb_var_dict_create(void);
struct PP_Var nb_var_dict_get(struct PP_Var, const char* key);
struct PP_Var nb_var_dict_get_var(struct PP_Var, struct PP_Var key);
NB_Bool nb_var_dict_set(struct PP_Var, const char* key, struct PP_Var);
NB_Bool nb_var_dict_set_var(struct PP_Var, struct PP_Var key, struct PP_Var);
struct PP_Var nb_var_dict_get_keys(struct PP_Var);
NB_Bool nb_var_dict_has_key(struct PP_Var, const char* key);

struct PP_Var nb_var_buffer_create(uint32_t);
uint32_t nb_var_buffer_byte_length(struct PP_Var);
void nb_var_buffer_map(struct PP_Var);
void nb_var_buffer_unmap(struct PP_Var);

struct PP_Var nb_var_int64_create(int64_t);

NB_Bool nb_var_int8(struct PP_Var, int8_t*);
NB_Bool nb_var_uint8(struct PP_Var, uint8_t*);
NB_Bool nb_var_int16(struct PP_Var, int16_t*);
NB_Bool nb_var_uint16(struct PP_Var, uint16_t*);
NB_Bool nb_var_int32(struct PP_Var, int32_t*);
NB_Bool nb_var_uint32(struct PP_Var, uint32_t*);
NB_Bool nb_var_int64(struct PP_Var, int64_t*);
NB_Bool nb_var_uint64(struct PP_Var, uint64_t*);
NB_Bool nb_var_float(struct PP_Var, float*);
NB_Bool nb_var_double(struct PP_Var, double*);
NB_Bool nb_var_string(struct PP_Var, const char**, uint32_t* out_length);
NB_Bool nb_var_function_id(struct PP_Var, int32_t* out_id);
NB_Bool nb_var_tagged_array(struct PP_Var,
                            const char** out_tag,
                            uint32_t* out_tag_length,
                            uint32_t* out_array_length);

NB_Bool nb_var_tagged_array_check(struct PP_Var,
                                  const char* expected_tag,
                                  uint32_t expected_array_len);

#ifdef __cplusplus
}
#endif

#endif /* NB_VAR_H_ */
