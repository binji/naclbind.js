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

// DO NOT EDIT, this file is auto-generated from //templates/glue.js

var naclbind = require('naclbind');

Type = naclbind.Type;
Tags = {};
Types = {};

[[for type in collector.types_topo:]]
[[  if type.kind == TypeKind.TYPEDEF:]]
{{type.js_inline}} = Type.Typedef('{{type.GetName()}}', {{type.get_canonical().js_inline}});
[[  elif type.kind == TypeKind.RECORD:]]
{{type.js_inline}} = Type.Record('{{type.GetName()}}', [
[[    for name, ftype, offset in type.fields():]]
  {name: '{{name}}', type: {{ftype.js_inline}}, offset: {{offset}}},
[[    ]]
], {{type.get_size()}});
[[  elif type.kind == TypeKind.ENUM:]]
{{type.js_inline}} = Type.Enum('{{type.GetName()}}'}});
[[  ]]
[[]]

[[for type, _ in collector.SortedFunctionTypes():]]
// {{type.get_canonical().spelling}}
var FuncType_{{type.js_mangle}} = Type.Function(
  {{type.get_result().get_canonical().js_inline}},
  [
[[  for arg_type in type.argument_types():]]
    {{arg_type.get_canonical().js_inline}},
[[  ]]
[[  if type.is_function_variadic():]]
  ], Type.VARIADIC
[[  else:]]
  ]
[[  ]]
);
[[]]

m = naclbind.Module();

[[for i, fn in enumerate(collector.functions):]]
m.defineFunction({{i+1}}, '{{fn.spelling}}', FuncType_{{fn.type.get_canonical().js_mangle}});
[[]]

m.Types = Types;
m.Tags = Tags;

module.exports = m;
