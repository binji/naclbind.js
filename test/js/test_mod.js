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

var chai = require('chai'),
    assert = chai.assert,
    assertTypesEqual = require('./equals').assertTypesEqual,
    naclbind = require('../../src/js/naclbind'),
    NaClEmbed = require('./nacl_embed_for_testing'),
    Embed = naclbind.Embed,
    Long = naclbind.Long,
    mod = naclbind.mod,
    type = naclbind.type;

chai.config.includeStack = true;

describe('Module', function() {
  it('should start with an empty message', function() {
    var m = mod.Module();
    assert.deepEqual(m.$getMessage(), {});
  });

  it('should throw when defining a function with id ERROR_IF_ID', function() {
    var fnType = type.Function(type.void, []);

    assert.throws(function() {
      mod.Function(mod.ERROR_IF_ID, fnType);
    }, /\$errorIf/);
  });

  it('should work for a simple function type', function() {
    var addType = type.Function(type.int, [type.int, type.int]),
        m = mod.Module();

    m.$defineFunction('add', [mod.Function(1, addType)]);

    m.add(3, 4);

    assert.deepEqual(m.$getMessage(), {
      set: {
        1: 3,
        2: 4
      },
      commands: [
        {id: 1, args: [1, 2], ret: 3}
      ]
    });
  });

  it('should work for an overloaded function', function() {
    var addIntType = type.Function(type.int, [type.int, type.int]),
        addFloatType = type.Function(type.float, [type.float, type.float]),
        m = mod.Module();

    m.$defineFunction('add', [
        mod.Function(1, addIntType),
        mod.Function(2, addFloatType)
    ]);

    m.add(3, 4);
    m.add(3.5, 4);

    assert.deepEqual(m.$getMessage(), {
      set: {
        1: 3,
        2: 4,
        4: 3.5,
        5: 4
      },
      commands: [
        {id: 1, args: [1, 2], ret: 3},
        {id: 2, args: [4, 5], ret: 6}
      ]
    });
  });

  it('should allow handle pipelining', function() {
    var voidp = type.Pointer(type.void),
        intp = type.Pointer(type.int),
        mallocType = type.Function(voidp, [type.uint]),
        getIntType = type.Function(type.int, [intp]),
        m = mod.Module();

    m.$defineFunction('malloc', [mod.Function(1, mallocType)]);
    m.$defineFunction('get', [mod.Function(2, getIntType)]);

    m.get(m.malloc(4).cast(intp));

    assert.deepEqual(m.$getMessage(), {
      set: { 1: 4 },
      commands: [
        {id: 1, args: [1], ret: 2},
        {id: 2, args: [2], ret: 3}
      ]
    });
  });

  it('should allow creation of another context', function() {
    var m = mod.Module(),
        c = m.$createContext(),
        h;

    m.$context = c;
    h = m.$handle(1000);

    assert.strictEqual(h.id, 1);
    assert.strictEqual(h.context, c);
    assert.strictEqual(c.handles.length, 1);
    assert.strictEqual(c.handles[0].id, 1);
  });

  describe('$commit', function() {
    it('should call callback and extract value from handle', function(done) {
      var ne = NaClEmbed(),
          e = Embed(ne),
          m = mod.Module(e),
          addType = type.Function(type.int, [type.int, type.int]),
          h;

      m.$defineFunction('add', [mod.Function(1, addType)]);

      ne.load();
      ne.setPostMessageCallback(function(msg) {
        assert.deepEqual(msg, {
          id: 1,
          get: [3],
          set: {1: 3, 2: 4},
          commands: [ {id: 1, args: [1, 2], ret: 3} ]
        });

        ne.message({id: 1, values: [7]});
      });

      h = m.add(3, 4);
      m.$commit([h], function(hVal) {
        assert.strictEqual(hVal, 7);
        done();
      });

      assert.deepEqual(m.$getMessage(), {});
    });

    it('should unwrap multiple handles', function(done) {
      var ne = NaClEmbed(),
          e = Embed(ne),
          m = mod.Module(e),
          addType = type.Function(type.int, [type.int, type.int]),
          h1,
          h2;

      m.$defineFunction('add', [mod.Function(1, addType)]);

      ne.load();
      ne.setPostMessageCallback(function(msg) {
        assert.deepEqual(msg.get, [3, 5]);
        ne.message({id: 1, values: [7, 12]});
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
      var ne = NaClEmbed(),
          e = Embed(ne),
          m = mod.Module(e),
          c = m.$createContext(),
          oldC;

      ne.load();
      ne.setPostMessageCallback(function(msg) {
        ne.message({id: 1});
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
      var ne = NaClEmbed(),
          e = Embed(ne),
          m = mod.Module(e),
          getLongLongType = type.Function(type.longlong, []),
          h;

      m.$defineFunction('getLongLong', [mod.Function(1, getLongLongType)]);

      ne.load();
      ne.setPostMessageCallback(function(msg) {
        assert.deepEqual(msg, {
          id: 1,
          get: [1],
          commands: [ {id: 1, args: [], ret: 1} ]
        });

        ne.message({id: 1, values: [[0, 256]]});
      });

      h = m.getLongLong();
      m.$commit([h], function(hVal) {
        assert.ok(hVal instanceof Long);
        assert.ok(hVal.equals(Long.fromBits(0, 256)));
        done();
      });
    });
  });

  describe('$destroyHandles', function() {
    it('should only destroy handles that are in the context', function() {
      var m = mod.Module(),
          c = m.$createContext();

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
      var m = mod.Module(),
          oldC = m.$context,
          c = m.$createContext();

      m.$handle(1);
      m.$handle(2);
      m.$context = c;
      m.$handle(3);
      m.$handle(4);

      m.$destroyHandles(oldC);
      assert.deepEqual(m.$getMessage().destroy, [1, 2]);
    });

    it('should remove handles from the context immediately', function() {
      var m = mod.Module(),
          c = m.$context;

      m.$handle(1);
      assert.strictEqual(c.handles.length, 1);

      m.$destroyHandles();
      assert.strictEqual(c.handles.length, 0);
    });

    it('should not destroy handles created after', function() {
      var m = mod.Module();

      m.$handle(1);
      m.$destroyHandles();
      m.$handle(2);
      assert.deepEqual(m.$getMessage().destroy, [1]);
      assert.strictEqual(m.$context.handles.length, 1);
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
      var ne = NaClEmbed(),
          e = Embed(ne),
          m = mod.Module(e);

      ne.load();
      ne.setPostMessageCallback(function(msg) {
        assert.deepEqual(msg.destroy, [1]);
        ne.message({id: 1});
      });

      m.$handle(1);
      m.$commitDestroy([], function() {
        done();
      });
    });

    it('should allow getting a handle that is being destroyed', function(done) {
      var ne = NaClEmbed(),
          e = Embed(ne),
          m = mod.Module(e),
          h;

      ne.load();
      ne.setPostMessageCallback(function(msg) {
        assert.deepEqual(msg.destroy, [1]);
        ne.message({id: 1, values: [1]});
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
        set: {1: 1},
        commands: [
          {id: mod.ERROR_IF_ID, args: [1]}
        ]
      });
    });

    it('should throw if called with non-convertible to int value', function() {
      var m = mod.Module(),
          rec = type.Record('rec', 4, type.STRUCT),
          h;

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
      var l = Long.ONE,
          two = Long.fromInt(2),
          i;
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
      var l = Long.NEG_ONE,
          two = Long.fromInt(2),
          i;
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
      var charp = type.Pointer(type.char.qualify(type.CONST));
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
      var two_to_the_fortieth = Long.fromNumber(Math.pow(2, 40)),
          neg_two_to_the_fortieth = two_to_the_fortieth.negate();
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

  describe('Handle', function() {
    describe('create', function() {
      it('should allow creation of handles', function() {
        var m = mod.Module(),
            h1 = m.$handle(4),
            h2 = m.$handle(4, type.float);

        assertTypesEqual(h1.type, type.schar);
        assertTypesEqual(h2.type, type.float);

        assert.deepEqual(m.$getMessage(), {
          set: {
            1: 4,
            2: 4
          },
        });
      });

      it('should allow use of explicitly-created handles', function() {
        var addType = type.Function(type.int, [type.int, type.int]),
            m = mod.Module(),
            h;

        m.$defineFunction('add', [mod.Function(1, addType)]);
        h = m.$handle(4);
        m.add(h, h);

        assert.deepEqual(m.$getMessage(), {
          set: { 1: 4 },
          commands: [
            {id: 1, args: [1, 1], ret: 2}
          ]
        });
      });

      it('should allow creation of int handles', function() {
        var m = mod.Module(),
            h1 = m.$handle(0),
            h2 = m.$handle(1000);

        assertTypesEqual(h1.type, type.schar);
        assertTypesEqual(h2.type, type.short);

        assert.deepEqual(m.$getMessage(), {
          set: {
            1: 0,
            2: 1000,
          },
        });
      });


      it('should allow creation of float handles', function() {
        var m = mod.Module(),
            h1 = m.$handle(Infinity),
            h2 = m.$handle(1e10);

        assertTypesEqual(h1.type, type.float);
        assertTypesEqual(h2.type, type.float);

        assert.deepEqual(m.$getMessage(), {
          set: {
            1: Infinity,
            2: 1e10,
          },
        });
      });

      it('should allow creation of string handles', function() {
        var m = mod.Module(),
            h = m.$handle("Hello");

        assertTypesEqual(h.type, type.Pointer(type.char.qualify(type.CONST)));

        assert.deepEqual(m.$getMessage(), {
          set: {
            1: "Hello",
          },
        });
      });

      it('should allow creation of null handles', function() {
        var m = mod.Module(),
            h = m.$handle(null);

        assertTypesEqual(h.type, type.Pointer(type.void));

        assert.deepEqual(m.$getMessage(), {
          set: {
            1: null,
          },
        });
      });

      it('should allow creation of long handles', function() {
        var m = mod.Module(),
            two_to_the_fortieth = Long.fromBits(0, 256),
            h = m.$handle(two_to_the_fortieth);

        assertTypesEqual(h.type, type.longlong);

        assert.deepEqual(m.$getMessage(), {
          set: {
            1: [0, 256],
          },
        });
      });
    });

    describe('cast', function() {
      it('should create a new handle with the same id', function() {
        var m = mod.Module(),
            h1,
            h2;

        h1 = m.$handle(1);
        h2 = h1.cast(type.long);

        assert.strictEqual(h1.id, h2.id);
        assert.deepEqual(h2.type, type.long);
      });

      it('should throw if the cast is invalid', function() {
        var m = mod.Module(),
            h;

        h = m.$handle(1);
        assert.throws(function() {
          h.cast(type.Function(type.void, []));
        }, /Invalid cast/);
      });
    });

    describe('setFinalizer', function() {
      it('should be called when the handle is destroyed', function(done) {
        var m = mod.Module(),
            h;

        h = m.$handle(1);
        h.setFinalizer(function(handle) {
          assert.strictEqual(handle, h);
          done();
        });

        m.$destroyHandles();
      });

      it('should be called in reverse order of creation', function(done) {
        var m = mod.Module(),
            finalizer,
            count = 0,
            h1,
            h2,
            h3,
            h4;

        finalizer = function(handle) {
          switch (count++) {
            case 0:
              assert.strictEqual(handle.id, 2);
              break;
            case 1:
              assert.strictEqual(handle.id, 1);
              break;
            case 2:
              assert.strictEqual(handle.id, 4);
              break;
            case 3:
              assert.strictEqual(handle.id, 3);
              done();
              break;
          }
        };

        h1 = m.$handle(1);
        h1.setFinalizer(finalizer);
        h2 = m.$handle(2);
        h2.setFinalizer(finalizer);
        m.$destroyHandles();

        // Should always be in reverse handle order, even if finalizers are
        // added in the opposite order.

        h3 = m.$handle(3);
        h4 = m.$handle(4);
        h4.setFinalizer(finalizer);
        h3.setFinalizer(finalizer);
        m.$destroyHandles();
      });

      it('should only be called once', function() {
        var m = mod.Module(),
            count = 0,
            h1,
            h2;

        h1 = m.$handle(1);
        h1.setFinalizer(function() { count++; });
        h2 = h1.cast(type.int);
        m.$destroyHandles();
        assert.strictEqual(count, 1);
        assert.deepEqual(m.$getMessage().destroy, [1]);
      });

      it('should throw if setFinalizer is called more than once', function() {
        var m = mod.Module(),
            dummy = function() {},
            h1,
            h2,
            h3;

        h1 = m.$handle(1);
        h1.setFinalizer(dummy);

        assert.throws(function() {
          h1.setFinalizer(dummy);
        }, /already has finalizer/);

        h2 = h1.cast(type.int);
        assert.throws(function() {
          h2.setFinalizer(dummy);
        }, /already has finalizer/);

        h3 = h2.cast(type.float);
        assert.throws(function() {
          h3.setFinalizer(dummy);
        }, /already has finalizer/);
      });

      it('should allow function calls from the finalizer', function() {
        var voidp = type.Pointer(type.void),
            mallocType = type.Function(voidp, [type.uint]),
            freeType = type.Function(type.void, [voidp]),
            m = mod.Module(),
            h;

        m.$defineFunction('malloc', [mod.Function(1, mallocType)]);
        m.$defineFunction('free', [mod.Function(2, freeType)]);

        h = m.malloc(4);
        h.setFinalizer(function(x) { m.free(x); });
        m.$destroyHandles();

        assert.deepEqual(m.$getMessage(), {
          set: { 1: 4 },
          destroy: [1, 2],
          commands: [
            {id: 1, args: [1], ret: 2},
            {id: 2, args: [2]}
          ]
        });
      });
    });
  });
});
