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

(function() {

describe('Test', function() {
  this.timeout(10000);

  var m;

  before(function() {
    var nmf = '/base/out/test/integration/rot13/rot13.nmf',
        mimeType = 'application/x-nacl';

    m = rot13Module(nmf, mimeType);
  });

  function rot13(s, cb) {
    var p = m.malloc(s.length + 1);
    m.memcpy(p, s, s.length + 1);
    m.rot13(p, s.length);
    var v = m.char_to_var(p);
    m.var_release(v);
    m.free(p);
    m.$commitDestroy([v], cb);
  }

  it('should work', function(done) {
    rot13('Hello', function(result) {
      assert.strictEqual(result, 'Uryyb');
      done();
    });
  });
});

})();

