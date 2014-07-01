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

[[[
from helper import *
from commands import *

types, functions = FixTypes(types, functions, add_builtin_types=False)
]]]

#include <stdlib.h>

#include "type.h"

static const char* kTypeString[] = {
  "(null)",
[[for _, type in types.no_builtins.iteritems():]]
[[  if not type.is_alias:]]
  /* {{type.id}} */ "{{type}}",
[[for _, type in types.function_types.iteritems():]]
[[  if not type.is_alias:]]
  /* {{type.id}} */ "{{type}}",
[[]]
};

const char* BuiltinTypeToString(Type id) {
  if (id <= 0 || id >= NUM_BUILTIN_TYPES) {
    return "<unknown>";
  }
  return kTypeString[id];
}
