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

describe('QueuingEmbed', function() {
  it('should not post messages before loaded', function() {
    var e = Embed('nmf', 'application/x-pnacl'),
        qe = QueuingEmbed(e);

    e.setPostMessageCallback(function() {
      assert.ok(false, 'Unexpected call to postMessage');
    });

    qe.postMessage({test: 'hello'});
  });

  it('should post all messages after load', function(done) {
    var e = Embed('nmf', 'application/x-pnacl'),
        qe = QueuingEmbed(e),
        loaded = false,
        callCount = 0;

    e.setPostMessageCallback(function(msg) {
      assert.equal(loaded, true, 'postMessage called before embed loaded');
      if (callCount === 0) {
        assert.deepEqual(msg, {test: 'hello'});
      } else if (callCount === 1) {
        assert.deepEqual(msg, {test: 'world'});
        done();
      } else {
        assert.ok(false, 'postMessage called too many times');
      }
      ++callCount;
    });

    e.addLoadListener(function() {
      loaded = true;
    });

    qe.postMessage({test: 'hello'});
    qe.postMessage({test: 'world'});
    e.load();
  });

  it('should delegate to internal embed', function(done) {
    var e = Embed('nmf', 'application/x-pnacl'),
        qe = QueuingEmbed(e);

    qe.addLoadListener(function() {});
    qe.addMessageListener(function() {});
    qe.addErrorListener(function() {});
    qe.addCrashListener(function() {
      assert.equal(qe.exitStatus, 1);
      done();
    });
    qe.appendToBody();
    qe.load();
    qe.exit(1);
  });
});
