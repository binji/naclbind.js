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

var assert = require('chai').assert,
    naclbind = require('../../src/js/naclbind'),
    type = naclbind.type,
    utils = naclbind.utils;

function helper(t1, t2, memo) {
  if (t1.kind !== t2.kind) {
    return false;
  }

  if (memo.indexOf(t1) !== -1 && memo.indexOf(t2) !== -1) {
    return true;
  }

  if (t1.cv !== t2.cv) {
    return false;
  }

  memo.push(t1);
  memo.push(t2);

  switch (t1.kind) {
    case type.POINTER:
      return helper(t1.pointee, t2.pointee, memo);
    case type.RECORD:
      return t1.tag === t2.tag &&
             t1.size === t2.size &&
             t1.isUnion === t2.isUnion &&
             utils.everyArrayPair(t1.fields, t2.fields, function(f1, f2) {
               return f1.name === f2.name &&
                      f1.offset === f2.offset &&
                      helper(f1.type, f2.type, memo);
             });
    case type.ENUM:
      return t1.tag === t2.tag;
    case type.TYPEDEF:
      return t1.tag === t2.tag &&
             helper(t1.alias, t2.alias, memo);
    case type.FUNCTIONPROTO:
      return helper(t1.resultType, t2.resultType, memo) &&
             utils.everyArrayPair(t1.argTypes, t2.argTypes, function(s1, s2) {
               return helper(s1, s2, memo);
             });
    case type.FUNCTIONNOPROTO:
      return helper(t1.resultType, t2.resultType, memo);
    case type.CONSTANTARRAY:
      return t1.arraySize === t2.arraySize &&
             helper(t1.elementType, t2.elementType, memo);
    case type.INCOMPLETEARRAY:
      return helper(t1.elementType, t2.elementType, memo);

    default:
      return true;
  }
}

function typesEqual(t1, t2) {
  return helper(t1, t2, [], []);
}

function assertTypesEqual(t1, t2) {
  assert.ok(typesEqual(t1, t2),
            'types aren\'t equal: ' + t1.spelling + ' != ' + t2.spelling);
}

module.exports = {
  typesEqual: typesEqual,
  assertTypesEqual: assertTypesEqual,
};
