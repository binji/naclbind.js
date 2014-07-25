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
});
