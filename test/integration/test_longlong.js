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

// UMD-style loader copied from:
// https://github.com/umdjs/umd/blob/master/returnExports.js
(function (root, factory) {
  if (typeof exports === 'object') {
    module.exports = factory();
  } else {
    factory().runTest();
  }
}(this, function () {

  function runTest() {
    describe('Test', function() {
      this.timeout(10000);

      it('should work', function(done) {
        var nmf = '/base/out/test/integration/longlong/longlong.nmf';
        var mimeType = 'application/x-nacl';
        var m = longlongModule.create(nmf, mimeType);
        var Long = longlongModule.Long;
        var h;

        h = m.add(Long.fromInt(0x10000), Long.fromNumber(0x100000000));
        m.$commit([h], function(hVal) {
          assert.ok(Long.fromNumber(0x100010000).equals(hVal));
          done();
        });
      });
    });
  }

  return {
    runTest: runTest
  };

}));
