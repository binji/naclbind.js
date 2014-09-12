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

var assert = require('assert'),
    QueuingEmbed = require('../src/js/queuing_embed'),
    Embed = require('./embed_for_testing');

function createEmbed() {
  return Embed('nmf', 'application/x-nacl');
}

describe('QueuingEmbed', function() {
  it('should not post messages before loaded', function() {
    var e = createEmbed(),
        qe = QueuingEmbed(e);

    e.setPostMessageCallback(function() {
      assert.ok(false, 'Unexpected call to postMessage');
    });

    qe.postMessage({test: 'hello'});
  });

  it('should post all messages after load', function(done) {
    var e = createEmbed(),
        qe = QueuingEmbed(e),
        loaded = false,
        callCount = 0,
        notCalled = function() { assert.ok(false, 'Shouldn\'t be called'); }

    e.setPostMessageCallback(function(msg) {
      assert.equal(loaded, true, 'postMessage called before embed loaded');
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

    e.addLoadListener(function() {
      loaded = true;
    });

    qe.postMessage({test: 'hello'}, notCalled);
    qe.postMessage({test: 'world'}, notCalled);
    e.load();
  });

  it('should call callback when message is posted from module', function(done) {
    var e = createEmbed(),
        qe = QueuingEmbed(e);

    e.setPostMessageCallback(function(msg) {
      assert.deepEqual(msg, {id: 1, msg: 'ping'});
      // When we get a ping, respond with a pong.
      e.message({id: 1, msg: 'pong'});
    });

    e.load();
    qe.postMessage({msg: 'ping'}, function(msg) {
      assert.deepEqual(msg, {id: 1, msg: 'pong'});
      done();
    });
  });

  it('should use incrementing ids for messages', function(done) {
    var e = createEmbed(),
        qe = QueuingEmbed(e),
        n = 1;

    e.setPostMessageCallback(function(msg) {
      assert.deepEqual(msg, {id: n});
      e.message({id: n});
    });

    e.load();

    qe.postMessage({}, function onMessage(msg) {
      assert.deepEqual(msg, {id: n});
      if (msg.id >= 100) {
        done();
        return;
      }

      ++n;
      qe.postMessage({}, onMessage);
    });
  });
});
