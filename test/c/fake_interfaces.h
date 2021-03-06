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

#ifndef FAKE_INTERFACES_H_
#define FAKE_INTERFACES_H_

#include <ppapi/c/pp_var.h>
#include "bool.h"
#include <ppapi/c/pp_var.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*PostMessageCallback)(struct PP_Var message, void* user_data);

void fake_interface_init(void);
void fake_interface_destroy(void);
void fake_interface_set_post_message_callback(PostMessageCallback,
                                              void* user_data);
NB_Bool fake_interface_check_no_references(void);
const void* fake_get_browser_interface(const char* interface_name);

#ifdef __cplusplus
}
#endif

#endif  // FAKE_INTERFACES_H_
