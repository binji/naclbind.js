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
    root.{{module_name}} = factory();
  }
}(this, function () {

{{IncludeFile('js/naclbind.js')}}

var tags = {},
    types = {};

[[for type in collector.types_topo:]]
[[  if type.kind == TypeKind.TYPEDEF:]]
{{type.js_spelling}} = type.Typedef('{{type.name}}', {{type.alias_type.js_spelling}});
[[  elif type.kind == TypeKind.RECORD:]]
[[    if type.is_union:]]
[[      record_type = 'type.UNION']]
[[    else:]]
[[      record_type = 'type.STRUCT']]
[[    ]]
{{type.js_spelling}} = type.Record('{{type.js_tag}}', {{type.size}}, {{record_type}});
[[  elif type.kind == TypeKind.ENUM:]]
{{type.js_spelling}} = type.Enum('{{type.js_tag}}');
[[  ]]
[[]]

[[for type in collector.types_topo:]]
[[  if type.kind == TypeKind.RECORD:]]
[[    for name, ftype, offset in type.fields:]]
{{type.js_spelling}}.addField('{{name}}', {{ftype.js_spelling}}, {{offset}});
[[    ]]
[[]]

[[for type, fns in collector.SortedFunctionTypes():]]
// {{type.canonical.c_spelling}} -- {{', '.join(fn.spelling for fn in fns)}}
[[  if type.kind == TypeKind.FUNCTIONPROTO:]]
var funcType_{{type.mangled}} = type.Function(
  {{type.result_type.canonical.js_spelling}},
  [
[[    for arg_type in type.arg_types:]]
    {{arg_type.canonical.js_spelling}},
[[  ]]
[[    if type.is_variadic:]]
  ], type.VARIADIC
[[    else:]]
  ]
[[    ]]
[[  elif type.kind == TypeKind.FUNCTIONNOPROTO:]]
var funcType_{{type.mangled}} = type.FunctionNoProto(
  {{type.result_type.canonical.js_spelling}}
[[  else:]]
[[    raise Error('Unexpected function type: %s' % type.kind)]]
[[  ]]
);
[[]]

function createModule(nmf, mimeType) {
  var embed,
      m;

  if (typeof nmf === 'undefined' ||
      typeof mimeType === 'undefined') {
    // Warn if we are running in a browser. Passing no nmf or mimetype is only
    // supported for testing.
    if (typeof window !== 'undefined') {
      console.log('WARNING: created module with empty nmf or mimetype.');
    }
  } else {
    embed = Embed(NaClEmbed(nmf, mimeType));
    embed.appendToBody();
  }

  m = mod.Module(embed);

[[for fn_name, fns in collector.SortedRemappedFunctions():]]
[[  if len(fns) == 1:]]
  m.$defineFunction('{{fn_name}}', [mod.Function({{fns[0].fn_id}}, funcType_{{fns[0].type.canonical.mangled}})]);
[[  else:]]
  m.$defineFunction('{{fn_name}}', [
[[    for fn in fns:]]
    mod.Function({{fn.fn_id}}, funcType_{{fn.type.canonical.mangled}}),
[[    ]]
  ]);
[[]]

  m.$types = types;
  m.$tags = tags;

  return m;
}

return {
  create: createModule,
  type: type
};

}));
