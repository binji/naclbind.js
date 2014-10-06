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

#ifndef HANDLE_H_
#define HANDLE_H_

#include <stdint.h>
#include <ppapi/c/pp_var.h>

#ifndef NB_ONE_FILE
#include "bool.h"
#include "type.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t Handle;

/* Size to use for default promotion (i.e. when passing to variadic functions */
#ifdef __x86_64__
typedef uint64_t nb_vararg_int_t;
typedef double nb_vararg_dbl_t;
#else
typedef uint32_t nb_vararg_int_t;
typedef double nb_vararg_dbl_t;
#endif

int32_t nb_handle_count(void);
bool nb_handle_register_int8(Handle, int8_t);
bool nb_handle_register_uint8(Handle, uint8_t);
bool nb_handle_register_int16(Handle, int16_t);
bool nb_handle_register_uint16(Handle, uint16_t);
bool nb_handle_register_int32(Handle, int32_t);
bool nb_handle_register_uint32(Handle, uint32_t);
bool nb_handle_register_int64(Handle, int64_t);
bool nb_handle_register_uint64(Handle, uint64_t);
bool nb_handle_register_float(Handle, float);
bool nb_handle_register_double(Handle, double);
bool nb_handle_register_voidp(Handle, void*);
bool nb_handle_register_var(Handle, struct PP_Var);
bool nb_handle_get_int8(Handle, int8_t*);
bool nb_handle_get_uint8(Handle, uint8_t*);
bool nb_handle_get_int16(Handle, int16_t*);
bool nb_handle_get_uint16(Handle, uint16_t*);
bool nb_handle_get_int32(Handle, int32_t*);
bool nb_handle_get_uint32(Handle, uint32_t*);
bool nb_handle_get_int64(Handle, int64_t*);
bool nb_handle_get_uint64(Handle, uint64_t*);
bool nb_handle_get_float(Handle, float*);
bool nb_handle_get_double(Handle, double*);
bool nb_handle_get_voidp(Handle, void**);
bool nb_handle_get_charp(Handle, char**);
bool nb_handle_get_var(Handle, struct PP_Var*);
bool nb_handle_get_default(Handle,
                           nb_vararg_int_t** iargs, nb_vararg_int_t* max_iargs,
                           nb_vararg_dbl_t** dargs, nb_vararg_dbl_t* max_dargs);
void nb_handle_destroy(Handle);
void nb_handle_destroy_many(Handle*, uint32_t handles_count);
bool nb_handle_convert_to_var(Handle, struct PP_Var*);

#ifdef __cplusplus
}
#endif

#endif  /* HANDLE_H_ */
