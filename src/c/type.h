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

#ifndef NB_TYPE_H_
#define NB_TYPE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  NB_TYPE_INVALID,
  NB_TYPE_INT8,
  NB_TYPE_UINT8,
  NB_TYPE_INT16,
  NB_TYPE_UINT16,
  NB_TYPE_INT32,
  NB_TYPE_UINT32,
  NB_TYPE_INT64,
  NB_TYPE_UINT64,
  NB_TYPE_FLOAT,
  NB_TYPE_DOUBLE,
  NB_TYPE_VOID_P,
  NB_TYPE_FUNC_P,
  NB_TYPE_VAR,
  NB_NUM_BUILTIN_TYPES
};
typedef int32_t NB_Type;

const char* nb_type_to_string(NB_Type);

#ifdef __cplusplus
}
#endif

#endif /* NB_TYPE_H_ */
