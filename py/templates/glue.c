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

[[[
from helper import *
from commands import *

types, functions = FixTypes(types, functions)
]]]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
[[for header in system_headers:]]
#include <{{header}}>
[[]]

[[for header in headers:]]
#include "{{header}}"
[[]]

#include "bool.h"
#include "commands.h"
#include "error.h"
#include "interfaces.h"
#include "message.h"
#include "type.h"
#include "var.h"

enum {
  TYPE_{{name.upper()}}_FIRST = {{helper.FIRST_ID}} - 1,
[[for type in types.no_builtins.itervalues():]]
[[  if not type.is_alias:]]
  TYPE_{{CamelToMacro(type.c_ident)}} = {{type.id}},
[[for type in types.function_types.itervalues():]]
[[  if type.is_alias:]]
  TYPE_FUNC_{{CamelToMacro(type.c_ident)}} = TYPE_FUNC_{{CamelToMacro(type.alias_of.c_ident)}},
[[  else:]]
  TYPE_FUNC_{{CamelToMacro(type.c_ident)}} = {{type.id}},
[[]]
  NUM_TYPES
};

static const char* kTypeString[] = {
[[for _, type in types.no_builtins.iteritems():]]
[[  if not type.is_alias:]]
  /* {{type.id}} */ "{{type}}",
[[for _, type in types.function_types.iteritems():]]
[[  if not type.is_alias:]]
  /* {{type.id}} */ "{{type}}",
[[]]
};

const char* TypeToString(Type id) {
  if (id < NUM_BUILTIN_TYPES) {
    return BuiltinTypeToString(id);
  } else if (id >= NUM_TYPES) {
    return "<unknown>";
  } else {
    return kTypeString[id - NUM_BUILTIN_TYPES];
  }
}

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
[[for fn in functions:]]
  {"{{fn.c_ident}}", Handle_{{fn.c_ident}}},
[[]]
  {NULL, NULL},
};

bool Handle{{Titlecase(name)}}Command(Command* command) {
  NameFunc* name_func = &g_FuncMap[0];
  for (; name_func->name; name_func++) {
    if (strcmp(name_func->name, command->command) == 0) {
      name_func->func(command);
      return TRUE;
    }
  }
  return FALSE;
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
      {{fn.c_ident}}({{ArgsCommaSep(fn_type.arg_types)}});
[[      else:]]
      {{fn_type.return_type}} result = ({{fn_type.return_type}}){{fn.c_ident}}({{ArgsCommaSep(fn_type.arg_types)}});
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
