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
var naclbind = require('../../src/js/naclbind');
var NaClEmbed = require('./nacl_embed_for_testing');
var Embed = naclbind.Embed;

chai.config.includeStack = true;

describe('Embed', function() {
  it('should not post messages before loaded', function() {
    var ne = NaClEmbed();
    var e = Embed(ne);

    ne.setPostMessageCallback(function() {
      assert.ok(false, 'Unexpected call to postMessage');
    });

    e.postMessageWithResponse({id: 1, test: 'hello'});
  });

  it('should post all messages after load', function(done) {
    var ne = NaClEmbed();
    var e = Embed(ne);
    var loaded = false;
    var callCount = 0;
    var notCalled = function() { assert.ok(false, 'Shouldn\'t be called'); }

    ne.setPostMessageCallback(function(msg) {
      assert.strictEqual(loaded, true,
                         'postMessage called before embed loaded');
      if (callCount === 0) {
        assert.deepEqual(msg, {id: 1, test: 'hello'});
      } else if (callCount === 1) {
        assert.deepEqual(msg, {id: 2, test: 'world'});
        done();
      } else {
        assert.ok(false, 'postMessage called too many times');
      }
      ++callCount;
    });

    ne.addLoadListener(function() {
      loaded = true;
    });

    e.postMessageWithResponse({id: 1, test: 'hello'}, notCalled);
    e.postMessageWithResponse({id: 2, test: 'world'}, notCalled);
    ne.load();
  });

  it('should call callback when message is posted from module', function(done) {
    var ne = NaClEmbed();
    var e = Embed(ne);

    ne.setPostMessageCallback(function(msg) {
      assert.deepEqual(msg, {id: 1, msg: 'ping'});
      // When we get a ping, respond with a pong.
      ne.message({id: 1, msg: 'pong'});
    });

    ne.load();
    e.postMessageWithResponse({id: 1, msg: 'ping'}, function(msg) {
      assert.deepEqual(msg, {id: 1, msg: 'pong'});
      done();
    });
  });

  it('should use incrementing ids for messages', function(done) {
    var ne = NaClEmbed();
    var e = Embed(ne);
    var n = 1;

    ne.setPostMessageCallback(function(msg) {
      assert.deepEqual(msg, {id: n});
      ne.message({id: n});
    });

    ne.load();

    e.postMessageWithResponse({id: n}, function onMessage(msg) {
      assert.deepEqual(msg, {id: n});
      if (msg.id >= 100) {
        done();
        return;
      }

      ++n;
      e.postMessageWithResponse({id: n}, onMessage);
    });
  });

  it('should allow registering callbacks', function(done) {
    var ne = NaClEmbed();
    var e = Embed(ne);

    ne.setPostMessageCallback(function(msg) {
      switch (msg.id) {
        case 1:
          assert.deepEqual(msg, {id: 1});
          // Call callback.
          ne.message({id: 2, cbId: 1, values: [10]});
          break;

        case 2:
          // Got result of callback.
          assert.deepEqual(msg, {id: 2, cbId: 1, values: [20]});
          done();
          break;
      }
    });

    ne.load();

    e.registerCallback(2, function(msg) {
      // Check callback message.
      assert.deepEqual(msg, {id: 2, cbId: 1, values: [10]});
      // Return "result" of callback to module.
      e.postMessage({id: 2, cbId: 1, values: [20]});
      e.destroyCallback(2);
    });

    e.postMessageWithResponse({id: 1});
  });

  var fireEventsImmediately = true;

  it('should throw if the message is not an object', function() {
    var ne = NaClEmbed(fireEventsImmediately);
    var e = Embed(ne);

    assert.throws(function() {
      ne.message(4);
    });
  });

  it('should throw if the id is not an integer', function() {
    var ne = NaClEmbed(fireEventsImmediately);
    var e = Embed(ne);

    assert.throws(function() {
      ne.message({id: '1'});
    }, /bad id/);
  });

  it('should throw if the cbId is not an integer', function() {
    var ne = NaClEmbed(fireEventsImmediately);
    var e = Embed(ne);

    assert.throws(function() {
      ne.message({id: 1, cbId: 3.5});
    }, /bad cbId/);
  });

  it('should throw if the id has no callback', function() {
    var ne = NaClEmbed(fireEventsImmediately);
    var e = Embed(ne);

    assert.throws(function() {
      ne.message({id: 1});
    }, /callback/);
  });

  it('should throw if postMessage has non-integer id', function() {
    var ne = NaClEmbed();
    var e = Embed(ne);

    assert.throws(function() {
      e.postMessage({});
    }, /id/);

    assert.throws(function() {
      e.postMessageWithResponse({}, function() {});
    }, /id/);
  });

  it('should throw if postMessage has non-integer or missing cbId', function() {
    var ne = NaClEmbed();
    var e = Embed(ne);

    assert.throws(function() {
      e.postMessage({id: 1, cbId: 'foo'});
    }, /cbId/);

    assert.throws(function() {
      e.postMessage({id: 1});  // missing cbId
    }, /cbId/);

    // postMessage is not queued because it should only be called from a
    // callback. As such, it is an error if it is called before the module is
    // loaded.
    assert.throws(function() {
      e.postMessage({id: 1, cbId: 1});
    }, /loaded/);
  });
});
