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

#ifndef NB_QUEUE_H_
#define NB_QUEUE_H_

#include <ppapi/c/pp_var.h>

#ifdef __cplusplus
extern "C" {
#endif

struct NB_Queue;

struct NB_Queue* nb_queue_create(int max_size);
void nb_queue_destroy(struct NB_Queue*);
int nb_queue_enqueue(struct NB_Queue*, struct PP_Var);
struct PP_Var nb_queue_dequeue(struct NB_Queue*);

#ifdef __cplusplus
}
#endif

#endif /* NB_QUEUE_H_ */
