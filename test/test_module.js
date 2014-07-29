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

function doNothing() {}

describe('Module', function() {
  var m, t, el;
  beforeEach(function() {
    m = nacl.Module('foo.nmf', 'application/x-pnacl', {
      name: 'foo',
      embedElementConstructor: EmbedElement,
      log: doNothing,
    });

    el = m.element;
    t = m.types;
  });

  it('should not post messages before it is loaded', function() {
    el.setPostMessageCallback(function(msg) {
      assert.fail(undefined, undefined, "Unexpected postMessage");
    });
    m.commit(doNothing);
  });

  it('should post queued messages after it is loaded', function(done) {
    el.setPostMessageCallback(function(msg) {
      assert.equal(el.loaded, true);
      done();
    });
    m.commit(doNothing);
    el.load();
  });

  describe('commit', function() {
    it('should throw without a callback', function() {
      el.setPostMessageCallback(doNothing);
      el.load();
      assert.throws(function() { m.commit(); });
    });

    it('should throw if called with non-handle', function() {
      el.setPostMessageCallback(doNothing);
      el.load();
      assert.throws(function() { m.commit(0, doNothing); });
    });

    it('should post message (0 cmds)', function(done) {
      el.load();
      el.setPostMessageCallback(function(msg) {
        assert.deepEqual(msg, {
          id: 1,
          commands: [],
          handles: []
        });
        done();
      });
      m.commit(doNothing);
    });

    it('should post message (1 cmd)', function(done) {
      el.load();
      el.setPostMessageCallback(function(msg) {
        assert.deepEqual(msg, {
          id: 1,
          commands: [{
            cmd: 'add',
            type: msg.commands[0].type,
            args: [3, 4],
            argIsHandle: [false, false],
            ret: 1
          }],
          handles: []
        });
        done();
      });

      var c = m.makeContext();
      c.add(3, 4);
      m.commit(doNothing);
    });

    it('should post message (1 cmd, 1 handle)', function(done) {
      el.load();
      el.setPostMessageCallback(function(msg) {
        assert.deepEqual(msg, {
          id: 1,
          commands: [{
            cmd: 'add',
            type: msg.commands[0].type,
            args: [3, 4],
            argIsHandle: [false, false],
            ret: 1
          }],
          handles: [1]
        });
        done();
      });

      var c = m.makeContext();
      var sum = c.add(3, 4);
      assert.equal(sum.id, 1);
      m.commit(sum, doNothing);
    });

    it('should post message (2 cmds, 2 handles)', function(done) {
      el.load();
      el.setPostMessageCallback(function(msg) {
        assert.deepEqual(msg, {
          id: 1,
          commands: [{
            cmd: 'add',
            type: msg.commands[0].type,
            args: [3, 4],
            argIsHandle: [false, false],
            ret: 1
          }, {
            cmd: 'sub',
            type: msg.commands[1].type,
            args: [1, 5],
            argIsHandle: [true, false],
            ret: 2
          }],
          handles: [1, 2]
        });
        done();
      });

      var c = m.makeContext();
      var sum1 = c.add(3, 4);
      var sum2 = c.sub(sum1, 5);
      assert.equal(sum1.id, 1);
      assert.equal(sum2.id, 2);
      m.commit(sum1, sum2, doNothing);
    });


    it('should call commit callback after response', function(done) {
      var gotPostMessage = false;
      el.load();
      el.setPostMessageCallback(function(msg) {
        gotPostMessage = true;
        el.message({id: msg.id, values: []});
      });
      m.commit(function() {
        assert(gotPostMessage);
        done();
      });
    });

    it('should respond with handle values', function(done) {
      el.load();
      el.setPostMessageCallback(function(msg) {
        el.message({
          id: msg.id,
          values: [7]
        });
      });

      var c = m.makeContext();
      var sum = c.add(3, 4);
      m.commit(sum, function(error, sum) {
        assert.equal(error, null);
        assert.equal(sum, 7);
        done();
      });
    });
  });

  describe('types', function() {
    it('should allow the creation of pointer types', function() {
      assert.equal(t.foo, undefined);
      m.makePointerType(1000, 'foo', t.int32$);
      assert(!t.foo.equals(t.int32$));
      assert(t.foo.baseType.equals(t.int32$));
      assert(t.foo.id, 1000);
    });

    it('should allow the creation of struct types', function() {
      assert.equal(t.foo, undefined);
      m.makeStructType(1000, 'foo', 8, {
        'field1': {type: t.char, offset: 0},
        'field2': {type: t.float32, offset: 4},
      });
      assert.equal(Object.keys(t.foo.fields).length, 2);
      assert.equal(t.foo.size, 8);
      assert(t.foo.fields.field1.type.equals(t.char));
      assert.equal(t.foo.fields.field1.offset, 0);
      assert(t.foo.fields.field2.type.equals(t.float32));
      assert.equal(t.foo.fields.field2.offset, 4);
      assert.equal(t.foo.id, 1000);
    });

    it('should allow the creation of alias types', function() {
      assert.equal(t.foo, undefined);
      m.makeAliasType('foo', t.int32);
      assert(t.foo.equals(t.int32));
    });

    it('should allow the creation of function types', function() {
      // Define a function with signature void(float, char*);
      // The signature must be unique; that is, two function types cannot have
      // the same signature but different ids.
      // So we pick a signature that is unlikely to conflict with builtins.
      // TODO(binji): better way to do this?
      var ftype = m.makeFunctionType(1000, t.void, t.float32, t.char$);
      assert.equal(ftype.id, 1000);
      assert(ftype.retType.equals(t.void));
      assert.equal(ftype.argTypes.length, 2);
      assert(ftype.argTypes[0].equals(t.float32));
      assert(ftype.argTypes[1].equals(t.char$));
    });

    it('should allow the creation of functions', function() {
      assert.equal(m.functions.foo, undefined);
      var ftype = m.makeFunctionType(1000, t.void, t.float32, t.char$);
      m.makeFunction('foo', ftype);
      assert.equal(typeof m.functions.foo, 'function');
    });

    it('should allow the creation of functions w/ overloads', function() {
      assert.equal(m.functions.foo, undefined);
      var ftype = m.makeFunctionType(1000, t.void, t.float32, t.char$);
      var ftype2 = m.makeFunctionType(1001, t.void, t.float64, t.int32$);
      m.makeFunction('foo', [ftype, ftype2]);
      assert.equal(typeof m.functions.foo, 'function');
    });
  });
});
