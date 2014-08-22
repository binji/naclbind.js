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

/* DO NOT EDIT, this file is auto-generated from //templates/type.h */

[[[
from helper import *
from commands import *

types, functions = FixTypes(types, functions, add_builtin_types=False)
]]]

#ifndef TYPE_H_
#define TYPE_H_

enum {
  TYPE_NONE = 0,
[[for type in types.no_builtins.itervalues():]]
[[  if not type.is_alias:]]
  TYPE_{{CamelToMacro(type.c_ident)}} = {{type.id}},
[[for type in types.function_types.itervalues():]]
[[  if type.is_alias:]]
  TYPE_FUNC_{{CamelToMacro(type.c_ident)}} = TYPE_FUNC_{{CamelToMacro(type.alias_of.c_ident)}},
[[  else:]]
  TYPE_FUNC_{{CamelToMacro(type.c_ident)}} = {{type.id}},
[[]]
  NUM_BUILTIN_TYPES
};
typedef int32_t Type;

const char* BuiltinTypeToString(Type);
const char* TypeToString(Type);  // Defined in module.

#endif  // TYPE_H_
