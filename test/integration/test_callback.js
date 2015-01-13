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
        var nmf = '/base/out/test/integration/callback/callback.nmf';
        var mimeType = 'application/x-nacl';
        var m = callbackModule.create(nmf, mimeType);
        var h1, h2;

        h1 = m.call_with_10_and_add_1(function(x, result) {
          result(x * 2);
        });

        h2 = m.call_with_10_and_add_1(function(x, result) {
          result(-x);
        });

        m.$commitDestroy([h1, h2], function(h1Val, h2Val) {
          assert.strictEqual(h1Val, 21);
          assert.strictEqual(h2Val, -9);
          done();
        });
      });

      it('should call callback multiple times', function(done) {
        var nmf = '/base/out/test/integration/callback/callback.nmf';
        var mimeType = 'application/x-nacl';
        var m = callbackModule.create(nmf, mimeType);
        var h;

        h = m.call_n_down_to_1_and_sum(10, function(x, result) {
          result(x + 1);
        });

        m.$commitDestroy([h], function(hVal) {
          assert.strictEqual(hVal, 65);
          done();
        });
      });

      it('should call callback with multiple arguments', function(done) {
        var nmf = '/base/out/test/integration/callback/callback.nmf';
        var mimeType = 'application/x-nacl';
        var m = callbackModule.create(nmf, mimeType);
        var data;
        var correct = 0;

        data = m.init_data(10, function(x, result) {
          result([7, 5, 4, 3, 10, 9, 1, 8, 6, 2][x]);
        });

        m.selection_sort(10, data, function(x, y, result) {
          result(x - y);
        });

        m.iter_data(10, data, function(index, value, result) {
          if (value === index + 1) {
            correct++;
          }
          result();
        });

        m.$commitDestroy([], function() {
          assert.strictEqual(correct, 10);
          done();
        });
      });
    });
  }

  return {
    runTest: runTest
  };

}));
