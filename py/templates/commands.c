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

/* DO NOT EDIT, this file is auto-generated from //py/templates/commands.c */

[[[
from helper import *
from commands import *

types, functions = FixTypes(types, functions, add_builtin_types=False)
]]]

#include "commands.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bool.h"
#include "builtin_funcs.h"
#include "error.h"
#include "interfaces.h"
#include "type.h"
#include "var.h"

static void Handle_destroyHandles(Command* command);

[[for fn in functions:]]
static void Handle_{{fn.c_ident}}(Command* command);
[[]]

typedef void (*HandleFunc)(Command*);
typedef struct {
  const char* name;
  HandleFunc func;
} NameFunc;

// TODO(binji): hashmap
static NameFunc g_FuncMap[] = {
  {"*destroyHandles", Handle_destroyHandles},
[[for fn in functions:]]
  {"{{fn.c_ident}}", Handle_{{fn.c_ident}}},
[[]]
  {NULL, NULL},
};

bool HandleBuiltinCommand(Command* command) {
  NameFunc* name_func = &g_FuncMap[0];
  for (; name_func->name; name_func++) {
    if (strcmp(name_func->name, command->command) == 0) {
      name_func->func(command);
      return TRUE;
    }
  }
  return FALSE;
}

void Handle_destroyHandles(Command* command) {
  int32_t index;
  int32_t handle_count = GetCommandArgCount(command);
  // TODO(binji): use malloc if handle_count is large?
  Handle* handles = alloca(handle_count * sizeof(Handle));
  for (index = 0; index < handle_count; ++index) {
    Arg* arg;
    if (!GetCommandArg(command, index, &arg)) {
      CMD_VERROR("Can't get arg %d", index);
      return;
    }

    if (!arg->is_handle) {
      CMD_VERROR("Expected arg %d to be a Handle", index);
      return;
    }

    int32_t arg_handle_int;
    if (!GetVarInt32(&arg->var, &arg_handle_int)) {
      CMD_VERROR("Expected handle arg %d to be int32_t", index);
      return;
    }

    Handle handle = arg_handle_int;
    handles[index] = handle;
  }

  DestroyHandles(handles, handle_count);
  printf("destroyHandles()\n");
}

[[for fn in functions:]]
void Handle_{{fn.c_ident}}(Command* command) {
[[  if len(fn.types) == 1:]]
  TYPE_CHECK(TYPE_FUNC_{{CamelToMacro(fn.c_ident)}});
[[    for arg_ix, arg_type in enumerate(fn.types[0].arg_types):]]
  {{ArgInit(arg_ix, arg_type)}}
[[    ]]
[[    if fn.types[0].return_type.is_void:]]
  {{fn.c_ident}}({{ArgsCommaSep(fn.types[0].arg_types)}});
[[    else:]]
  {{fn.types[0].return_type}} result = ({{fn.types[0].return_type}}){{fn.c_ident}}({{ArgsCommaSep(fn.types[0].arg_types)}});
  {{RegisterHandle(fn.types[0].return_type)}}
[[    ]]
  {{PrintFunction(fn.c_ident, fn.types[0])}}
[[  else:]]
  switch (command->type) {
[[    for fn_type in fn.types:]]
    case TYPE_FUNC_{{CamelToMacro(fn_type.c_ident)}}: {
[[      for arg_ix, arg_type in enumerate(fn_type.arg_types):]]
      {{ArgInit(arg_ix, arg_type)}}
[[      ]]
[[      if fn_type.return_type.is_void:]]
      {{fn_type.c_ident}}({{ArgsCommaSep(fn_type.arg_types)}});
[[      else:]]
      {{fn_type.return_type}} result = ({{fn_type.return_type}}){{fn_type.c_ident}}({{ArgsCommaSep(fn_type.arg_types)}});
      {{RegisterHandle(fn_type.return_type)}}
[[      ]]
      {{PrintFunction(fn.c_ident, fn_type)}}
      break;
    }
[[    ]]
    default:
      TYPE_FAIL;
      break;
  }
[[  ]]
}

[[]]
