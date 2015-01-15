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

var chai = require('chai');
var assert = chai.assert;
var assertTypesEqual = require('./equals').assertTypesEqual;
var naclbind = require('../../src/js/naclbind');
var NaClEmbed = require('./nacl_embed_for_testing');
var Embed = naclbind.Embed;
var Long = naclbind.Long;
var mod = naclbind.mod;
var type = naclbind.type;

chai.config.includeStack = true;

describe('Module', function() {
  it('should start with an empty message', function() {
    var m = mod.Module();
    assert.deepEqual(m.$getMessage(), {id: 1});
  });

  it('should throw when defining a function with id < 0', function() {
    var fnType = type.Function(type.void, []);

    assert.throws(function() { mod.Function(-1, fnType); });
    assert.throws(function() { mod.Function(-10, fnType); });
  });

  it('should work for a simple function type', function() {
    var addType = type.Function(type.int, [type.int, type.int]);
    var m = mod.Module();

    m.$defineFunction('add', [mod.Function(0, addType)]);

    m.add(3, 4);

    assert.deepEqual(m.$getMessage(), {
      id: 1,
      set: {
        1: 3,
        2: 4
      },
      commands: [
        {id: 0, args: [1, 2], ret: 3}
      ]
    });
  });

  it('should work for an overloaded function', function() {
    var addIntType = type.Function(type.int, [type.int, type.int]);
    var addFloatType = type.Function(type.float, [type.float, type.float]);
    var m = mod.Module();

    m.$defineFunction('add', [
        mod.Function(0, addIntType),
        mod.Function(1, addFloatType)
    ]);

    m.add(3, 4);
    m.add(3.5, 4);

    assert.deepEqual(m.$getMessage(), {
      id: 1,
      set: {
        1: 3,
        2: 4,
        4: 3.5,
        5: 4
      },
      commands: [
        {id: 0, args: [1, 2], ret: 3},
        {id: 1, args: [4, 5], ret: 6}
      ]
    });
  });

  it('should allow passing function pointers', function() {
    var pfunc = type.Pointer(type.Function(type.int, [type.int]));
    var getFuncType = type.Function(pfunc, []);
    var useFuncType = type.Function(type.void, [pfunc]);
    var m = mod.Module();
    var h;

    m.$defineFunction('getFunc', [mod.Function(0, getFuncType)]);
    m.$defineFunction('useFunc', [mod.Function(1, useFuncType)]);

    h = m.getFunc();
    m.useFunc(h);

    assert.deepEqual(m.$getMessage(), {
      id: 1,
      commands: [
        {id: 0, args: [], ret: 1},
        {id: 1, args: [1]}
      ]
    });
  });

  it('should allow handle pipelining', function() {
    var voidp = type.Pointer(type.void);
    var intp = type.Pointer(type.int);
    var mallocType = type.Function(voidp, [type.uint]);
    var getIntType = type.Function(type.int, [intp]);
    var m = mod.Module();

    m.$defineFunction('malloc', [mod.Function(0, mallocType)]);
    m.$defineFunction('get', [mod.Function(1, getIntType)]);

    m.get(m.malloc(4).$cast(intp));

    assert.deepEqual(m.$getMessage(), {
      id: 1,
      set: { 1: 4 },
      commands: [
        {id: 0, args: [1], ret: 2},
        {id: 1, args: [2], ret: 3}
      ]
    });
  });

  it('should allow creation of another context', function() {
    var m = mod.Module();
    var c = m.$createContext();
    var h;

    m.$context = c;
    h = m.$handle(1000);

    assert.strictEqual(h.$id, 1);
    assert.strictEqual(h.$context, c);
    assert.strictEqual(c.$handles.length, 1);
    assert.strictEqual(c.$handles[0].$id, 1);
  });

  describe('$commit', function() {
    it('should call callback and extract value from handle', function(done) {
      var ne = NaClEmbed();
      var e = Embed(ne);
      var m = mod.Module(e);
      var addType = type.Function(type.int, [type.int, type.int]);
      var h;

      m.$defineFunction('add', [mod.Function(0, addType)]);

      ne.$load();
      ne.$setPostMessageCallback(function(msg) {
        assert.deepEqual(msg, {
          id: 1,
          get: [3],
          set: {1: 3, 2: 4},
          commands: [ {id: 0, args: [1, 2], ret: 3} ]
        });

        ne.$message({id: 1, values: [7]});
      });

      h = m.add(3, 4);
      m.$commit([h], function(hVal) {
        assert.strictEqual(hVal, 7);
        done();
      });

      assert.deepEqual(m.$getMessage(), {id: 2});
    });

    it('should unwrap multiple handles', function(done) {
      var ne = NaClEmbed();
      var e = Embed(ne);
      var m = mod.Module(e);
      var addType = type.Function(type.int, [type.int, type.int]);
      var h1;
      var h2;

      m.$defineFunction('add', [mod.Function(0, addType)]);

      ne.$load();
      ne.$setPostMessageCallback(function(msg) {
        assert.deepEqual(msg.get, [3, 5]);
        ne.$message({id: 1, values: [7, 12]});
      });

      h1 = m.add(3, 4);
      h2 = m.add(h1, 5);
      m.$commit([h1, h2], function(h1Val, h2Val) {
        assert.strictEqual(h1Val, 7);
        assert.strictEqual(h2Val, 12);
        done();
      });
    });

    it('should pass current context to commit callback', function(done) {
      var ne = NaClEmbed();
      var e = Embed(ne);
      var m = mod.Module(e);
      var c = m.$createContext();
      var oldC;

      ne.$load();
      ne.$setPostMessageCallback(function(msg) {
        ne.$message({id: 1});
      });

      oldC = m.$context;
      m.$context = c;

      m.$commit([], function() {
        assert.strictEqual(m.$context, c);
        done();
      });

      // Change context back to default. The commit callback should still have
      // c.
      m.$context = oldC;
    });

    it('should convert longs back to objects in callback', function(done) {
      var ne = NaClEmbed();
      var e = Embed(ne);
      var m = mod.Module(e);
      var getLongLongType = type.Function(type.longlong, []);
      var h;

      m.$defineFunction('getLongLong', [mod.Function(0, getLongLongType)]);

      ne.$load();
      ne.$setPostMessageCallback(function(msg) {
        assert.deepEqual(msg, {
          id: 1,
          get: [1],
          commands: [ {id: 0, args: [], ret: 1} ]
        });

        ne.$message({id: 1, values: [['long', 0, 256]]});
      });

      h = m.getLongLong();
      m.$commit([h], function(hVal) {
        assert.ok(hVal instanceof Long);
        assert.ok(hVal.equals(Long.fromBits(0, 256)));
        done();
      });
    });

    it('should return errors to the caller', function(done) {
      var ne = NaClEmbed();
      var e = Embed(ne);
      var m = mod.Module(e);
      var getIntType = type.Function(type.int, []);
      var h;

      m.$defineFunction('getInt', [mod.Function(0, getIntType)]);

      ne.$load();
      ne.$setPostMessageCallback(function(msg) {
        assert.deepEqual(msg, {
          id: 1,
          set: {1: 1},
          get: [2],
          commands: [
            {id: -1, args: [1]},
            {id: 0, args: [], ret: 2}
          ]
        });

        ne.$message({id: 1, values: [undefined], error: 0});
      });

      m.$errorIf(1);
      h = m.getInt();
      m.$commit([h], function(error, hVal) {
        assert.strictEqual(error.failedAt, 0);
        assert.ok(error.stack);
        done();
      });
    });

    it('should push undefined error when everything works', function(done) {
      var ne = NaClEmbed();
      var e = Embed(ne);
      var m = mod.Module(e);
      var getIntType = type.Function(type.int, []);
      var h;

      m.$defineFunction('getInt', [mod.Function(0, getIntType)]);

      ne.$load();
      ne.$setPostMessageCallback(function(msg) {
        assert.deepEqual(msg, {
          id: 1,
          get: [1],
          commands: [
            {id: 0, args: [], ret: 1}
          ]
        });

        ne.$message({id: 1, values: [42]});
      });

      h = m.getInt();
      m.$commit([h], function(error, hVal) {
        assert.strictEqual(hVal, 42);
        assert.strictEqual(error, undefined);
        done();
      });
    });
  });

  describe('$destroyHandles', function() {
    it('should only destroy handles that are in the context', function() {
      var m = mod.Module();
      var c = m.$createContext();

      // Created in default context.
      m.$handle(1);
      m.$handle(2);
      m.$context = c;
      // Created in context |c|.
      m.$handle(3);
      m.$handle(4);

      // Destroy handles in current context.
      m.$destroyHandles();
      assert.deepEqual(m.$getMessage().destroy, [3, 4]);
    });

    it('should destroy handles from passed-in context', function() {
      var m = mod.Module();
      var oldC = m.$context;
      var c = m.$createContext();

      m.$handle(1);
      m.$handle(2);
      m.$context = c;
      m.$handle(3);
      m.$handle(4);

      m.$destroyHandles(oldC);
      assert.deepEqual(m.$getMessage().destroy, [1, 2]);
    });

    it('should remove handles from the context immediately', function() {
      var m = mod.Module();
      var c = m.$context;

      m.$handle(1);
      assert.strictEqual(c.$handles.length, 1);

      m.$destroyHandles();
      assert.strictEqual(c.$handles.length, 0);
    });

    it('should not destroy handles created after', function() {
      var m = mod.Module();

      m.$handle(1);
      m.$destroyHandles();
      m.$handle(2);
      assert.deepEqual(m.$getMessage().destroy, [1]);
      assert.strictEqual(m.$context.$handles.length, 1);
    });

    it('should accumulate handles to destroy', function() {
      var m = mod.Module();

      m.$handle(1);
      m.$destroyHandles();
      m.$handle(2);
      m.$destroyHandles();
      assert.deepEqual(m.$getMessage().destroy, [1, 2]);
    });
  });

  describe('$commitDestroy', function() {
    it('should be equivalent to calling destroy then commit', function(done) {
      var ne = NaClEmbed();
      var e = Embed(ne);
      var m = mod.Module(e);

      ne.$load();
      ne.$setPostMessageCallback(function(msg) {
        assert.deepEqual(msg.destroy, [1]);
        ne.$message({id: 1});
      });

      m.$handle(1);
      m.$commitDestroy([], function() {
        done();
      });
    });

    it('should allow getting a handle that is being destroyed', function(done) {
      var ne = NaClEmbed();
      var e = Embed(ne);
      var m = mod.Module(e);
      var h;

      ne.$load();
      ne.$setPostMessageCallback(function(msg) {
        assert.deepEqual(msg.destroy, [1]);
        ne.$message({id: 1, values: [1]});
      });

      h = m.$handle(1);
      m.$commitDestroy([h], function(hVal) {
        assert.strictEqual(hVal, 1);
        done();
      });
    });
  });

  describe('$errorIf', function() {
    it('should add a command with id of ERROR_IF_ID', function() {
      var m = mod.Module();

      m.$errorIf(1);
      assert.deepEqual(m.$getMessage(), {
        id: 1,
        set: {1: 1},
        commands: [
          {id: mod.ERROR_IF_ID, args: [1]}
        ]
      });
    });

    it('should throw if called with non-convertible to int value', function() {
      var m = mod.Module();
      var rec = type.Record('rec', 4, type.STRUCT);
      var h;

      // TODO(binji): this should be illegal. Come up with a better way to get
      // a value that isn't convertible to an int...
      h = m.$handle(null, rec);

      assert.throws(function() {
        m.$errorIf(h);
      }, /invalid type/);
    });
  });

  describe('numberToType', function() {
    it('should return smallest type for a given number', function() {
      assertTypesEqual(mod.numberToType(0), type.schar);
      assertTypesEqual(mod.numberToType(127), type.schar);
      assertTypesEqual(mod.numberToType(128), type.uchar);
      assertTypesEqual(mod.numberToType(255), type.uchar);
      assertTypesEqual(mod.numberToType(256), type.short);
      assertTypesEqual(mod.numberToType(32767), type.short);
      assertTypesEqual(mod.numberToType(32768), type.ushort);
      assertTypesEqual(mod.numberToType(2147483647), type.int);
      assertTypesEqual(mod.numberToType(2147483648), type.uint);
      assertTypesEqual(mod.numberToType(4294967295), type.uint);
      assertTypesEqual(mod.numberToType(-1), type.schar);
      assertTypesEqual(mod.numberToType(-128), type.schar);
      assertTypesEqual(mod.numberToType(-129), type.short);
      assertTypesEqual(mod.numberToType(-32768), type.short);
      assertTypesEqual(mod.numberToType(-32769), type.int);
      assertTypesEqual(mod.numberToType(-2147483648), type.int);
    });

    it('should return double if value is out of integer range', function() {
      assertTypesEqual(mod.numberToType(4294967297), type.double);
      assertTypesEqual(mod.numberToType(-2147483649), type.double);
    });

    it('should return float if value is non-finite', function() {
      assertTypesEqual(mod.numberToType(Infinity), type.float);
      assertTypesEqual(mod.numberToType(-Infinity), type.float);
      assertTypesEqual(mod.numberToType(NaN), type.float);
    });

    it('should return float/double if value has decimal component', function() {
      assertTypesEqual(mod.numberToType(3.5), type.float);
      assertTypesEqual(mod.numberToType(3.14159), type.double);
    });
  });

  describe('longToType', function() {
    it('should return smallest type for a given positive number', function() {
      var l = Long.ONE;
      var two = Long.fromInt(2);
      var i;
      assertTypesEqual(mod.longToType(Long.ZERO), type.schar);
      for (i = 1; i <= 7; ++i) {
        assertTypesEqual(mod.longToType(l), type.schar);
        l = l.multiply(two);
      }

      assertTypesEqual(mod.longToType(l), type.uchar);
      l = l.multiply(two);

      for (i = 9; i <= 15; ++i) {
        assertTypesEqual(mod.longToType(l), type.short);
        l = l.multiply(two);
      }

      assertTypesEqual(mod.longToType(l), type.ushort);
      l = l.multiply(two);

      for (i = 17; i <= 31; ++i) {
        assertTypesEqual(mod.longToType(l), type.int);
        l = l.multiply(two);
      }

      assertTypesEqual(mod.longToType(l), type.uint);
      l = l.multiply(two);

      for (i = 33; i <= 63; ++i) {
        assertTypesEqual(mod.longToType(l), type.longlong);
        l = l.multiply(two);
      }
    });

    it('should return smallest type for a given negative number', function() {
      var l = Long.NEG_ONE;
      var two = Long.fromInt(2);
      var i;
      for (i = 1; i <= 8; ++i) {
        assertTypesEqual(mod.longToType(l), type.schar);
        l = l.multiply(two);
      }

      for (i = 9; i <= 16; ++i) {
        assertTypesEqual(mod.longToType(l), type.short);
        l = l.multiply(two);
      }

      for (i = 17; i <= 32; ++i) {
        assertTypesEqual(mod.longToType(l), type.int);
        l = l.multiply(two);
      }

      for (i = 33; i <= 63; ++i) {
        assertTypesEqual(mod.longToType(l), type.longlong);
        l = l.multiply(two);
      }
    });
  });

  describe('objectToType', function() {
    it('should work for numbers', function() {
      assertTypesEqual(mod.objectToType(100), type.schar);
      assertTypesEqual(mod.objectToType(-2000), type.short);
      assertTypesEqual(mod.objectToType(159256), type.int);
      assertTypesEqual(mod.objectToType(3.5), type.float);
      assertTypesEqual(mod.objectToType(Infinity), type.float);
    });

    it('should work for strings', function() {
      var charp = type.Pointer(type.char.$qualify(type.CONST));
      assertTypesEqual(charp, mod.objectToType("hi"));
    });

    it('should return void* for null', function() {
      var voidp = type.Pointer(type.void);
      assertTypesEqual(voidp, mod.objectToType(null));
    });

    it('should work for bools', function() {
      assertTypesEqual(type.schar, mod.objectToType(true));
      assertTypesEqual(type.schar, mod.objectToType(false));
    });

    it('should work for 64-bit ints', function() {
      var two_to_the_fortieth = Long.fromNumber(Math.pow(2, 40));
      var neg_two_to_the_fortieth = two_to_the_fortieth.negate();
      assertTypesEqual(type.longlong, mod.objectToType(two_to_the_fortieth));
      assertTypesEqual(type.longlong, mod.objectToType(neg_two_to_the_fortieth));
      // TODO(binji): Fix unsigned long long. Long type represents s64 not u64.
      // assertTypesEqual(type.ulonglong, mod.objectToType(Long.MAX_VALUE));

      assertTypesEqual(type.schar, mod.objectToType(Long.ZERO));
    });

    it('should fail for anything else', function() {
      assert.throws(function() { mod.objectToType(undefined); });
      assert.throws(function() { mod.objectToType([1, 2]); });
      assert.throws(function() { mod.objectToType({a: 1}); });
      assert.throws(function() { mod.objectToType(new Date); });
    });
  });

  describe('$set', function() {
    var voidp = type.Pointer(type.void);
    var intp = type.Pointer(type.int);
    var floatp = type.Pointer(type.float);
    var mallocType = type.Function(voidp, [type.uint]);
    var setIntType = type.Function(type.void, [intp, type.int]);
    var setFloatType = type.Function(type.void, [floatp, type.float]);
    var addVoidType = type.Function(voidp, [voidp, type.int]);

    it('should provide syntatic sugar for setting a field', function() {
      var m = mod.Module();
      var s = type.Record('s', 8, type.STRUCT);
      var h;

      s.$addField('f', type.int, 0);
      s.$addField('g', type.float, 4);

      m.$defineFunction('malloc', [mod.Function(0, mallocType)]);
      // Necessary functions used by $set.
      m.$defineFunction('set', [mod.Function(1, setIntType),
                                mod.Function(2, setFloatType)]);
      m.$defineFunction('add', [mod.Function(3, addVoidType)]);

      h = m.malloc(s.$size);
      m.$set(h, s.f, 42);
      m.$set(h, s.g, 3.25);

      assert.deepEqual(m.$getMessage(), {
        id: 1,
        set: {
          1: 8,    // s.size
          3: 42,   // value to write into h.f
          4: 4,    // s.g offset
          6: 3.25  // value to write into h.g
        },
        commands: [
          {id: 0, args: [1], ret: 2},     // $2 = malloc(4);
          {id: 1, args: [2, 3]},          // *($2) = 42;
          {id: 3, args: [2, 4], ret: 5},  // $5 = $2 + 4;
          {id: 2, args: [5, 6]},          // *($5) = 3.25;
        ]
      });
    });

    it('should allow setting nested field', function() {
      var m = mod.Module();
      var s = type.Record('s', 4, type.STRUCT);
      var t = type.Record('t', 4, type.STRUCT);
      var h;

      s.$addField('i', type.int, 0);
      s.$addField('f', t, 4);
      t.$addField('g', type.int, 0);

      m.$defineFunction('malloc', [mod.Function(0, mallocType)]);
      // Necessary functions used by $set.
      m.$defineFunction('set', [mod.Function(1, setIntType)]);
      m.$defineFunction('add', [mod.Function(2, addVoidType)]);

      h = m.malloc(s.$size);
      m.$set(h, s.f.g, 42);

      assert.deepEqual(m.$getMessage(), {
        id: 1,
        set: {
          1: 4,   // s.size
          3: 4,   // s.f.g offset
          5: 42,  // value to write into s.f.g
        },
        commands: [
          {id: 0, args: [1], ret: 2},     // $2 = malloc(4);
          {id: 2, args: [2, 3], ret: 4},  // $4 = $2 + 4;
          {id: 1, args: [4, 5]},          // *($4) = 42;
        ]
      });
    });
  });

  describe('$get', function() {
    it('should provide syntatic sugar for getting a field', function() {
      var m = mod.Module();
      var voidp = type.Pointer(type.void);
      var intp = type.Pointer(type.int);
      var floatp = type.Pointer(type.float);
      var mallocType = type.Function(voidp, [type.uint]);
      var getIntType = type.Function(type.int, [intp]);
      var getFloatType = type.Function(type.float, [floatp]);
      var addVoidType = type.Function(voidp, [voidp, type.int]);
      var s = type.Record('s', 8, type.STRUCT);
      var h;

      s.$addField('f', type.int, 0);
      s.$addField('g', type.float, 4);

      m.$defineFunction('malloc', [mod.Function(0, mallocType)]);
      // Necessary functions used by $get.
      m.$defineFunction('get', [mod.Function(1, getIntType),
                                mod.Function(2, getFloatType)]);
      m.$defineFunction('add', [mod.Function(3, addVoidType)]);

      h = m.malloc(s.$size);
      m.$get(h, s.f);
      m.$get(h, s.g);

      assert.deepEqual(m.$getMessage(), {
        id: 1,
        set: {
          1: 8,    // s.size
          4: 4,    // s.g offset
        },
        commands: [
          {id: 0, args: [1], ret: 2},     // $2 = malloc(4);
          {id: 1, args: [2], ret: 3},     // $3 = *($2);
          {id: 3, args: [2, 4], ret: 5},  // $5 = $2 + 4;
          {id: 2, args: [5], ret: 6},     // $6 = *($5);
        ]
      });
    });
  });

  describe('Handle', function() {
    describe('create', function() {
      it('should allow creation of handles', function() {
        var m = mod.Module();
        var h1 = m.$handle(4);
        var h2 = m.$handle(4, type.float);

        assertTypesEqual(h1.$type, type.schar);
        assertTypesEqual(h2.$type, type.float);

        assert.deepEqual(m.$getMessage(), {
          id: 1,
          set: {
            1: 4,
            2: 4
          },
        });
      });

      it('should allow use of explicitly-created handles', function() {
        var addType = type.Function(type.int, [type.int, type.int]);
        var m = mod.Module();
        var h;

        m.$defineFunction('add', [mod.Function(0, addType)]);
        h = m.$handle(4);
        m.add(h, h);

        assert.deepEqual(m.$getMessage(), {
          id: 1,
          set: { 1: 4 },
          commands: [
            {id: 0, args: [1, 1], ret: 2}
          ]
        });
      });

      it('should allow creation of int handles', function() {
        var m = mod.Module();
        var h1 = m.$handle(0);
        var h2 = m.$handle(1000);

        assertTypesEqual(h1.$type, type.schar);
        assertTypesEqual(h2.$type, type.short);

        assert.deepEqual(m.$getMessage(), {
          id: 1,
          set: {
            1: 0,
            2: 1000,
          },
        });
      });


      it('should allow creation of float handles', function() {
        var m = mod.Module();
        var h1 = m.$handle(Infinity);
        var h2 = m.$handle(1e10);

        assertTypesEqual(h1.$type, type.float);
        assertTypesEqual(h2.$type, type.float);

        assert.deepEqual(m.$getMessage(), {
          id: 1,
          set: {
            1: Infinity,
            2: 1e10,
          },
        });
      });

      it('should allow creation of string handles', function() {
        var m = mod.Module();
        var h = m.$handle("Hello");

        assertTypesEqual(h.$type, type.Pointer(type.char.$qualify(type.CONST)));

        assert.deepEqual(m.$getMessage(), {
          id: 1,
          set: {
            1: "Hello",
          },
        });
      });

      it('should allow creation of null handles', function() {
        var m = mod.Module();
        var h = m.$handle(null);

        assertTypesEqual(h.$type, type.Pointer(type.void));

        assert.deepEqual(m.$getMessage(), {
          id: 1,
          set: {
            1: null,
          },
        });
      });

      it('should allow creation of long handles', function() {
        var m = mod.Module();
        var two_to_the_fortieth = Long.fromBits(0, 256);
        var h = m.$handle(two_to_the_fortieth);

        assertTypesEqual(h.$type, type.longlong);

        assert.deepEqual(m.$getMessage(), {
          id: 1,
          set: {
            1: ['long', 0, 256],
          },
        });
      });
    });

    describe('cast', function() {
      it('should create a new handle with the same id', function() {
        var m = mod.Module();
        var h1;
        var h2;

        h1 = m.$handle(1);
        h2 = h1.$cast(type.long);

        assert.strictEqual(h1.$id, h2.$id);
        assert.deepEqual(h2.$type, type.long);
      });

      it('should throw if the cast is invalid', function() {
        var m = mod.Module();
        var h;

        h = m.$handle(1);
        assert.throws(function() {
          h.$cast(type.Function(type.void, []));
        }, /Invalid cast/);
      });
    });

    describe('setFinalizer', function() {
      it('should be called when the handle is destroyed', function(done) {
        var m = mod.Module();
        var h;

        h = m.$handle(1);
        h.$setFinalizer(function(handle) {
          assert.strictEqual(handle, h);
          done();
        });

        m.$destroyHandles();
      });

      it('should be called in reverse order of creation', function(done) {
        var m = mod.Module();
        var finalizer;
        var count = 0;
        var h1;
        var h2;
        var h3;
        var h4;

        finalizer = function(handle) {
          switch (count++) {
            case 0:
              assert.strictEqual(handle.$id, 2);
              break;
            case 1:
              assert.strictEqual(handle.$id, 1);
              break;
            case 2:
              assert.strictEqual(handle.$id, 4);
              break;
            case 3:
              assert.strictEqual(handle.$id, 3);
              done();
              break;
          }
        };

        h1 = m.$handle(1);
        h1.$setFinalizer(finalizer);
        h2 = m.$handle(2);
        h2.$setFinalizer(finalizer);
        m.$destroyHandles();

        // Should always be in reverse handle order, even if finalizers are
        // added in the opposite order.

        h3 = m.$handle(3);
        h4 = m.$handle(4);
        h4.$setFinalizer(finalizer);
        h3.$setFinalizer(finalizer);
        m.$destroyHandles();
      });

      it('should only be called once', function() {
        var m = mod.Module();
        var count = 0;
        var h1;
        var h2;

        h1 = m.$handle(1);
        h1.$setFinalizer(function() { count++; });
        h2 = h1.$cast(type.int);
        m.$destroyHandles();
        assert.strictEqual(count, 1);
        assert.deepEqual(m.$getMessage().destroy, [1]);
      });

      it('should throw if setFinalizer is called more than once', function() {
        var m = mod.Module();
        var dummy = function() {};
        var h1;
        var h2;
        var h3;

        h1 = m.$handle(1);
        h1.$setFinalizer(dummy);

        assert.throws(function() {
          h1.$setFinalizer(dummy);
        }, /already has finalizer/);

        h2 = h1.$cast(type.int);
        assert.throws(function() {
          h2.$setFinalizer(dummy);
        }, /already has finalizer/);

        h3 = h2.$cast(type.float);
        assert.throws(function() {
          h3.$setFinalizer(dummy);
        }, /already has finalizer/);
      });

      it('should allow function calls from the finalizer', function() {
        var voidp = type.Pointer(type.void);
        var mallocType = type.Function(voidp, [type.uint]);
        var freeType = type.Function(type.void, [voidp]);
        var m = mod.Module();
        var h;

        m.$defineFunction('malloc', [mod.Function(0, mallocType)]);
        m.$defineFunction('free', [mod.Function(1, freeType)]);

        h = m.malloc(4);
        h.$setFinalizer(function(x) { m.free(x); });
        m.$destroyHandles();

        assert.deepEqual(m.$getMessage(), {
          id: 1,
          set: { 1: 4 },
          destroy: [1, 2],
          commands: [
            {id: 0, args: [1], ret: 2},
            {id: 1, args: [2]}
          ]
        });
      });
    });
  });

  describe('Callbacks', function() {
    it('should pass the correct format to the module', function() {
      var pfunc = type.Pointer(type.Function(type.int, [type.int]));
      var useFuncType = type.Function(type.void, [pfunc]);
      var callback = function(x) {};
      var ne = NaClEmbed();
      var e = Embed(ne);
      var m = mod.Module(e);
      var h;

      m.$defineFunction('useFunc', [mod.Function(0, useFuncType)]);

      m.useFunc(callback);

      assert.deepEqual(m.$getMessage(), {
        id: 1,
        set: {
          1: ['function', 2],
        },
        commands: [
          {id: 0, args: [1]}
        ]
      });
    });

    it('should call a JavaScript callback', function(done) {
      var pfunc = type.Pointer(type.Function(type.int, [type.int]));
      var useFuncType = type.Function(type.void, [pfunc]);
      var ne = NaClEmbed();
      var e = Embed(ne);
      var m = mod.Module(e);
      var callback;
      var commitCallbackCalled = false;

      m.$defineFunction('useFunc', [mod.Function(0, useFuncType)]);

      ne.$load();
      ne.$setPostMessageCallback(function(msg) {
        if (msg.id === 1) {
          // First call the commit callback.
          ne.$message({id: 1, values: []});
          // Then call callback.
          ne.$message({id: 2, cbId: 1, values: [42]});
        } else if (msg.id === 2) {
          // Return value from callback.
          assert.deepEqual(msg, {
            id: 2,
            cbId: 1,
            values: [13]
          });

          done();
        }
      });

      callback = function(val, result) {
        assert.strictEqual(commitCallbackCalled, true);
        assert.strictEqual(val, 42);
        result(13);
      };

      m.useFunc(callback);
      m.$commit([], function() {
        commitCallbackCalled = true;
      });
    });

    it('should work calling a JavaScript callback twice', function(done) {
      var pfunc = type.Pointer(type.Function(type.int, [type.int]));
      var useFuncType = type.Function(type.void, [pfunc]);
      var ne = NaClEmbed();
      var m = mod.Module(Embed(ne));
      var callback;

      m.$defineFunction('useFunc', [mod.Function(0, useFuncType)]);

      ne.$load();
      ne.$setPostMessageCallback(function(msg) {
        switch (msg.id) {
          case 1:
            // First call the commit callback.
            ne.$message({id: 1, values: []});
            // Then call callback.
            ne.$message({id: 2, cbId: 1, values: [10]});
            break;

          case 2:
            switch (msg.cbId) {
              case 1:
                assert.deepEqual(msg.values, [20]);
                // Call callback again.
                ne.$message({id: 2, cbId: 2, values: [42]});
                break;

              case 2:
                assert.deepEqual(msg.values, [84]);
                done();
                break;
            }
            break;
        }
      });

      m.useFunc(function(x, result) {
        result(x * 2);
      });

      m.$commit([], function() {});
    });

    it('should throw when calling result twice', function(done) {
      var pfunc = type.Pointer(type.Function(type.int, [type.int]));
      var useFuncType = type.Function(type.void, [pfunc]);
      var ne = NaClEmbed();
      var m = mod.Module(Embed(ne));

      m.$defineFunction('useFunc', [mod.Function(0, useFuncType)]);

      ne.$load();
      ne.$setPostMessageCallback(function(msg) {
        if (msg.id === 1) {
          // First call the commit callback.
          ne.$message({id: 1, values: []});
          // Then call callback.
          ne.$message({id: 2, cbId: 1, values: [42]});
        }
      });

      m.useFunc(function(val, result) {
        result(1);
        assert.throws(function() {
          result(2);
        });
        done();
      });
      m.$commit([], function() {});
    });

    it('should allow returning from callback', function(done) {
      var pfunc = type.Pointer(type.Function(type.int, []));
      var useFuncType = type.Function(type.void, [pfunc]);
      var ne = NaClEmbed();
      var m = mod.Module(Embed(ne));

      m.$defineFunction('useFunc', [mod.Function(0, useFuncType)]);

      ne.$load();
      ne.$setPostMessageCallback(function(msg) {
        if (msg.id === 1) {
          // First call the callback.
          ne.$message({id: 2, cbId: 1, values: []});
          // Then call the commit callback.
          ne.$message({id: 1, values: []});
        } else if (msg.id === 2){
          assert.deepEqual(msg.values, [10]);
          done();
        }
      });

      m.useFunc(function() { return 10; });
      m.$commit([], function() {});
    });
  });
});
