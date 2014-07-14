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

// DO NOT EDIT, this file is auto-generated from //py/templates/builtin.js

[[[
from helper import *
from commands import *

types, functions = FixTypes(types, functions, add_builtin_types=False)

def jsbool(x):
  return 'true' if x else 'false'
]]]

"use strict";

define([], function() {

  function init(module) {
    var m = module;
    var t = m.types;

[[for _, type in types.no_builtins.iteritems():]]
[[  if type.is_alias:]]
    m.makeAliasType('{{type.js_ident}}', t.{{type.alias_of.js_ident}});
[[  elif type.is_void:]]
    m.typeBuilder_.makeVoidType({{type.id}}, '{{type.js_ident}}');
[[  elif type.is_primitive:]]
    m.typeBuilder_.makePrimitiveType({{type.id}}, '{{type.js_ident}}', {{type.size}}, {{jsbool(type.is_signed)}}, {{jsbool(type.is_int)}});
[[  elif type.is_pepper:]]
    m.typeBuilder_.makePepperType({{type.id}}, '{{type.js_ident}}', {{type.js_prototype}});
[[  elif type.is_struct:]]
    m.makeStructType({{type.id}}, '{{type.js_ident}}', {{type.size}}, {
[[    for field in type.fields:]]
      {{field.name}}: {type: t.{{field.type.js_ident}}, offset: {{field.offset}}},
[[    ]]
    });
[[  elif type.is_pointer:]]
    m.makePointerType({{type.id}}, '{{type.js_ident}}', t.{{type.base_type.js_ident}});
[[]]

[[[
def ArgTypesString(fn_type):
  if fn_type.arg_types:
    return ', ' + CommaSep(['t.%s' % t.js_ident for t in fn_type.arg_types])
  return ''

def ReturnTypeString(fn_type):
  return ', t.' + fn_type.return_type.js_ident
]]]
[[for fn_type in types.function_types.itervalues():]]
[[  if fn_type.is_alias:]]
    var fnType_{{fn_type.js_ident}} = fnType_{{fn_type.alias_of.js_ident}};
[[  else:]]
    var fnType_{{fn_type.js_ident}} = m.makeFunctionType({{fn_type.id}}{{ReturnTypeString(fn_type)}}{{ArgTypesString(fn_type)}});
[[]]

[[for fn in functions:]]
[[  if len(fn.types) == 1:]]
    m.makeFunction('{{fn.js_ident}}', fnType_{{fn.js_ident}});
[[  else:]]
    m.makeFunction('{{fn.js_ident}}', [
[[    for fn_type in fn.types:]]
      fnType_{{fn_type.js_ident}},
[[    ]]
    ]);
[[]]
  }

  return {
    init: init
  };
});
