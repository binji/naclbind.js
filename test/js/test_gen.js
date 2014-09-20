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
    child_process = require('child_process'),
    path = require('path'),
    tmp = require('tmp'),
    execFile = child_process.execFile,
    type = require('../../src/js/type'),
    utils = require('../../src/js/utils');

describe('Generate JS', function() {
  function appendPath(s, path) {
    return s.split(':').concat([path]).join(':');
  }

  before(function() {
    // Hack NODE_PATH to include the source directory automatically.
    var srcDir = path.resolve(__dirname, '../../src/js'),
        nodePath = appendPath(process.env.NODE_PATH || '', srcDir);
    process.env.NODE_PATH = nodePath;
    require('module')._initPaths();
  });

  function gen(inpath, callback) {
    var genPy = path.resolve(__dirname, '../../bin/gen.py'),
        glueJs = path.resolve(__dirname, '../../templates/glue.js'),
        tmpOpts = {prefix: 'genjs', unsafeCleanup: true},
        oldCallback = callback;

    tmp.dir(tmpOpts, function(error, outdir, cleanup) {
      if (error) {
        return callback(error);
      }

      callback = function(error, genJs) {
        cleanup();
        oldCallback(error, genJs);
      };

      var outpath = outpath = path.join(outdir, 'gen.js'),
          cmd = [genPy, '-t', glueJs, inpath, '-o', outpath],
          execOpts = {cwd: __dirname};

      execFile('python', cmd, execOpts, function(error, stdout, stderr) {
        if (error) {
          return callback(error);
        }

        var genJs = require(outpath);
        callback(null, genJs);
      });
    });
  }

  this.slow(800);

  it('should pass basic tests', function(done) {
    gen('../data/simple.h', function(error, m) {
      if (error) {
        assert.ok(false, 'Error generating JS.\n' + error);
      }

      assert.strictEqual(1, m.$functionsCount);
      assert.ok(m.rot13);
      assert.strictEqual(1, m.rot13.types.length);

      var expected =
          type.Function(type.void, [type.Pointer(type.char), type.uint]);
      var actual = m.rot13.types[0];
      assert.ok(expected.equals(actual));

      assert.equal(0, m.$typesCount);
      assert.equal(0, m.$tagsCount);

      done();
    });
  });
});
