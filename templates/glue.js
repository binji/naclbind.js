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

// UMD-style loader copied from:
// https://github.com/umdjs/umd/blob/master/returnExports.js
(function (root, factory) {
  /* jshint undef: true */
  /* global define */
  if (typeof define === 'function' && define.amd) {
    // AMD. Register as an anonymous module.
    define([], factory);
  } else if (typeof exports === 'object') {
    // Node. Does not work with strict CommonJS, but
    // only CommonJS-like environments that support module.exports,
    // like Node.
    module.exports = factory();
  } else {
    // Browser globals (root is window)
    root.naclbind = factory();
  }
}(this, function () {

{{IncludeFile('js/naclbind.js')}}

var tags = {},
    types = {};

[[for type in collector.types_topo:]]
[[  if type.kind == TypeKind.UNEXPOSED:]]
[[    type = type.get_canonical()]]
[[  if type.kind == TypeKind.TYPEDEF:]]
{{type.js_inline}} = type.Typedef('{{type.GetName()}}', {{type.get_canonical().js_inline}});
[[  elif type.kind == TypeKind.RECORD:]]
{{type.js_inline}} = type.Record('{{type.GetName()}}', {{type.get_size()}}, [
[[    for name, ftype, offset in type.fields():]]
  type.Field('{{name}}', {{ftype.js_inline}}, {{offset}}),
[[    ]]
[[    if type.get_declaration().kind == CursorKind.UNION_DECL:]]
], type.UNION);
[[    else:]]
], type.STRUCT);
[[  elif type.kind == TypeKind.ENUM:]]
{{type.js_inline}} = type.Enum('{{type.GetName()}}');
[[  else:]]
// {{type.kind}} {{type.spelling}}
[[  ]]
[[]]

[[for type, fns in collector.SortedFunctionTypes():]]
// {{type.get_canonical().spelling}} -- {{', '.join(fn.spelling for fn in fns)}}
var funcType_{{type.js_mangle}} = type.Function(
  {{type.get_result().get_canonical().js_inline}},
  [
[[  for arg_type in type.argument_types():]]
    {{arg_type.get_canonical().js_inline}},
[[  ]]
[[  if type.is_function_variadic():]]
  ], type.VARIADIC
[[  else:]]
  ]
[[  ]]
);
[[]]

var m = mod.Module();

[[for i, fn in enumerate(collector.functions):]]
m.$defineFunction('{{fn.spelling}}', [mod.Function({{i+1}}, funcType_{{fn.type.get_canonical().js_mangle}})]);
[[]]

m.$types = types;
m.$tags = tags;

return m;

}));
