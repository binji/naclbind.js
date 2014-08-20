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

var nacl = require('../dist/2nacl'),
    EmbedElement = require('./test_embed_element'),
    assert = require('assert');

/*
describe('TypeBuilder', function() {
  var tb, t;
  beforeEach(function() {
    tb = new nacl.TypeBuilder;
    t = tb.nameHash;

  });

  describe('primitive', function() {
    it('create', function() {
      tb.makePrimitiveType(1, 'char', 1, true, true);
      assert.equal(t.char.name, 'char');
      assert.equal(t.char.size, 1);
      assert.equal(t.char.isSigned, true);
      assert.equal(t.char.isInt, true);
    });
  });

  describe('pointer', function() {
    it('create', function() {
      tb.makePrimitiveType(1, 'char', 1, true, true);
      tb.makePointerType(2, 'foo', t.char);
      assert(!t.foo.equals(t.char));
      assert(t.foo.baseType.equals(t.char));
      assert(t.foo.id, 2);
    });

    it('should throw if id is non-unique', function() {
      tb.makePrimitiveType(1, 'char', 1, true, true);
      assert.throws(function() {
        tb.makePointerType(1, 'foo', t.int32$);
      });
    });

    it('should throw if type is non-unique', function() {
      tb.makePrimitiveType(1, 'char', 1, true, true);
      tb.makePointerType(2, 'char$', t.char);
      assert.throws(function() {
        tb.makePointerType(3, 'foo', t.char);
      });
    });

    it('should throw if name is non-unique', function() {
      tb.makePrimitiveType(1, 'char', 1, true, true);
      tb.makePrimitiveType(2, 'int', 4, true, true);
      assert.throws(function() {
        tb.makePointerType(3, 'char', t.int);
      });
    });
  });

  describe('struct', function() {
    it('create', function() {
      tb.makePrimitiveType(1, 'char', 1, true, true);
      tb.makePrimitiveType(2, 'int', 4, true, true);
      tb.makeStructType(3, 'foo', 8, {
        'field1': {type: t.char, offset: 0},
        'field2': {type: t.int, offset: 4},
      });
      assert.equal(Object.keys(t.foo.fields).length, 2);
      assert.equal(t.foo.size, 8);
      assert(t.foo.fields.field1.type.equals(t.char));
      assert.equal(t.foo.fields.field1.offset, 0);
      assert(t.foo.fields.field2.type.equals(t.int));
      assert.equal(t.foo.fields.field2.offset, 4);
      assert.equal(t.foo.id, 3);
    });

    it('create w/o fields', function() {
      tb.makeStructType(1, 'foo', 0, {});
      assert.equal(Object.keys(t.foo.fields).length, 0);
      assert.equal(t.foo.size, 0);
      assert.equal(t.foo.id, 1);
    });

    it('should throw if id is non-unique', function() {
      tb.makePrimitiveType(1, 'char', 1, true, true);
      assert.throws(function() {
        tb.makeStructType(1, 'foo', 0, {});
      });
    });

    it('should throw if name is non-unique', function() {
      tb.makePrimitiveType(1, 'char', 1, true, true);
      assert.throws(function() {
        tb.makeStructType(1000, 'char', 0, {});
      });
    });
  });

  describe('alias', function() {
    it('create', function() {
      tb.makePrimitiveType(1, 'char', 1, true, true);
      tb.makeAliasType('foo', t.char);
      assert(t.foo.equals(t.char));
    });

    it('should throw if name is non-unique', function() {
      tb.makePrimitiveType(1, 'char', 1, true, true);
      tb.makePrimitiveType(2, 'int', 4, true, true);
      assert.throws(function() {
        tb.makeAliasType('int', t.char);
      });
    });
  });

  describe('function', function() {
    it('create type', function() {
      tb.makePrimitiveType(1, 'char', 1, true, true);
      // Function signature: char (*)(char);
      var ftype = tb.makeFunctionType(2, t.char, t.char);
      assert.equal(ftype.id, 2);
      assert(ftype.retType.equals(t.char));
      assert.equal(ftype.argTypes.length, 1);
      assert(ftype.argTypes[0].equals(t.char));
    });

    it('should throw if id is non-unique', function() {
      tb.makePrimitiveType(1, 'char', 1, true, true);
      assert.throws(function() {
        tb.makeFunctionType(1, t.char, t.char);
      });
    });

    it('should throw if signature is non-unique', function() {
      tb.makePrimitiveType(1, 'char', 1, true, true);
      tb.makePrimitiveType(2, 'int', 4, true, true);
      tb.makeFunctionType(3, t.int, t.char);
      assert.throws(function() {
        tb.makeFunctionType(4, t.int, t.char);
      });
    });
  });
});
*/
