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

#ifndef NB_HANDLE_H_
#define NB_HANDLE_H_

#include <stdint.h>
#include <ppapi/c/pp_var.h>

#ifndef NB_ONE_FILE
#include "bool.h"
#include "type.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t NB_Handle;

/* Size to use for default promotion (i.e. when passing to variadic functions */
#ifdef __x86_64__
typedef uint64_t NB_VarArgInt;
typedef double NB_VarArgDbl;
#else
typedef uint32_t NB_VarArgInt;
typedef double NB_VarArgDbl;
#endif

int32_t nb_handle_count(void);
NB_Bool nb_handle_register_int8(NB_Handle, int8_t);
NB_Bool nb_handle_register_uint8(NB_Handle, uint8_t);
NB_Bool nb_handle_register_int16(NB_Handle, int16_t);
NB_Bool nb_handle_register_uint16(NB_Handle, uint16_t);
NB_Bool nb_handle_register_int32(NB_Handle, int32_t);
NB_Bool nb_handle_register_uint32(NB_Handle, uint32_t);
NB_Bool nb_handle_register_int64(NB_Handle, int64_t);
NB_Bool nb_handle_register_uint64(NB_Handle, uint64_t);
NB_Bool nb_handle_register_float(NB_Handle, float);
NB_Bool nb_handle_register_double(NB_Handle, double);
NB_Bool nb_handle_register_voidp(NB_Handle, void*);
NB_Bool nb_handle_register_funcp(NB_Handle, void(*)(void));
NB_Bool nb_handle_register_var(NB_Handle, struct PP_Var);
NB_Bool nb_handle_get_int8(NB_Handle, int8_t*);
NB_Bool nb_handle_get_uint8(NB_Handle, uint8_t*);
NB_Bool nb_handle_get_int16(NB_Handle, int16_t*);
NB_Bool nb_handle_get_uint16(NB_Handle, uint16_t*);
NB_Bool nb_handle_get_int32(NB_Handle, int32_t*);
NB_Bool nb_handle_get_uint32(NB_Handle, uint32_t*);
NB_Bool nb_handle_get_int64(NB_Handle, int64_t*);
NB_Bool nb_handle_get_uint64(NB_Handle, uint64_t*);
NB_Bool nb_handle_get_float(NB_Handle, float*);
NB_Bool nb_handle_get_double(NB_Handle, double*);
NB_Bool nb_handle_get_voidp(NB_Handle, void**);
NB_Bool nb_handle_get_funcp(NB_Handle, void(**)(void));
NB_Bool nb_handle_get_charp(NB_Handle, char**);
NB_Bool nb_handle_get_var(NB_Handle, struct PP_Var*);
NB_Bool nb_handle_get_default(NB_Handle,
                              NB_VarArgInt** iargs,
                              NB_VarArgInt* max_iargs,
                              NB_VarArgDbl** dargs,
                              NB_VarArgDbl* max_dargs);
void nb_handle_destroy(NB_Handle);
void nb_handle_destroy_many(NB_Handle*, uint32_t handles_count);
NB_Bool nb_handle_convert_to_var(NB_Handle, struct PP_Var*);

#ifdef __cplusplus
}
#endif

#endif /* NB_HANDLE_H_ */
