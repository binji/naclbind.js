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
    naclbind = require('../../src/js/naclbind'),
    NaClEmbed = require('./nacl_embed_for_testing');
    Embed = naclbind.Embed;

chai.config.includeStack = true;

describe('Embed', function() {
  it('should not post messages before loaded', function() {
    var ne = NaClEmbed(),
        e = Embed(ne);

    ne.setPostMessageCallback(function() {
      assert.ok(false, 'Unexpected call to postMessage');
    });

    e.postMessage({test: 'hello'});
  });

  it('should post all messages after load', function(done) {
    var ne = NaClEmbed(),
        e = Embed(ne),
        loaded = false,
        callCount = 0,
        notCalled = function() { assert.ok(false, 'Shouldn\'t be called'); }

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

    e.postMessage({test: 'hello'}, notCalled);
    e.postMessage({test: 'world'}, notCalled);
    ne.load();
  });

  it('should call callback when message is posted from module', function(done) {
    var ne = NaClEmbed(),
        e = Embed(ne);

    ne.setPostMessageCallback(function(msg) {
      assert.deepEqual(msg, {id: 1, msg: 'ping'});
      // When we get a ping, respond with a pong.
      ne.message({id: 1, msg: 'pong'});
    });

    ne.load();
    e.postMessage({msg: 'ping'}, function(msg) {
      assert.deepEqual(msg, {id: 1, msg: 'pong'});
      done();
    });
  });

  it('should use incrementing ids for messages', function(done) {
    var ne = NaClEmbed(),
        e = Embed(ne),
        n = 1;

    ne.setPostMessageCallback(function(msg) {
      assert.deepEqual(msg, {id: n});
      ne.message({id: n});
    });

    ne.load();

    e.postMessage({}, function onMessage(msg) {
      assert.deepEqual(msg, {id: n});
      if (msg.id >= 100) {
        done();
        return;
      }

      ++n;
      e.postMessage({}, onMessage);
    });
  });
});
