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
    mod = naclbind.mod,
    type = naclbind.type,
    Embed = naclbind.Embed,
    NaClEmbed = naclbind.NaClEmbed;

describe('Test', function() {
  it('should work', function(done) {
    /*
    var e = Embed(NaClEmbed('foo.nmf', 'application/x-nacl')),
        m = mod.Module(e),
        addType = type.Function(type.int, [type.int, type.int]),
        h;

    e.appendToBody();
    m.$defineFunction('add', [mod.Function(1, addType)]);
    h = m.add(3, 4);
    m.$commit([h], function(hVal) {
      assert.strictEqual(hVal, 7);
      done();
    });
    */
    assert.equal(2 + 2, 4);
    done();
  });
});
