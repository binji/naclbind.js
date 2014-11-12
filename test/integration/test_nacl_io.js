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

      var m;

      before(function() {
        var nmf = '/base/out/test/integration/nacl_io/nacl_io.nmf';
        var mimeType = 'application/x-nacl';

        m = nacl_ioModule.create(nmf, mimeType);
      });

      it('should work', function(done) {
        var msg = "Hello, World!";
        m.nacl_io_init();
        m.mount("", "/memfs", "memfs", 0, null);
        var f = m.fopen("/memfs/foo.txt", "w+");
        f.$setFinalizer(m.fclose.bind(m));
        var wrote = m.fwrite(msg, 1, msg.length, f);
        m.$commit([wrote], function(wrote) {
          assert.strictEqual(wrote, msg.length);
          var p = m.malloc(20);
          p.$setFinalizer(m.free.bind(m));
          m.fseek(f, 0, 0);
          var read = m.fread(p, 1, 20, f);
          m.$commitDestroy([read], function(read) {
            assert.strictEqual(read, msg.length);
            done();
          });
        });
      });
    });
  }

  function buildOpts(defaultOpts) {
    defaultOpts.compile.libs.unshift('nacl_io');
  }

  return {
    buildOpts: buildOpts,
    runTest: runTest,
  };

}));
