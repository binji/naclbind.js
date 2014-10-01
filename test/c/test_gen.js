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
    nacl = require('nacl-sdk'),
    fs = require('fs'),
    gen = require('../shared/gen'),
    path = require('path');

function genAndRun(header, source, testSource, callback) {
  var basename = path.basename(testSource),
      outdir = path.resolve(__dirname, '../../out/test/c/test_gen', basename),
      glueC = path.join(outdir, 'glue.c'),
      infiles = [
        source,
        testSource,
        path.resolve(__dirname, 'test_gen.cc'),
        path.resolve(__dirname, 'fake_interfaces.c'),
        path.resolve(__dirname, 'json.cc'),
        path.resolve(__dirname, 'main.cc')
      ],
      opts = {
        toolchain: 'newlib',
        config: 'Debug',
        arch: 'x86_64',
        outdir: outdir,

        compile: {
          args: ['-Wall', '-Werror', '-pthread'],
          defines: ['NB_NO_APP'],
          includeDirs: [
            path.resolve(__dirname),
            path.resolve(__dirname, '..'),
            path.resolve(__dirname, '../../src/c')
          ],
          libs: [
            'jsoncpp', 'ppapi_simple', 'gtest', 'nacl_io', 'ppapi_cpp', 'ppapi'
          ]
        },
      };

  gen.file(header, glueC, 'glue.c', function(error, outfile) {
    if (error) {
      return callback(error);
    }

    infiles.push(outfile);

    nacl.build(infiles, basename, opts, function(error, nexe) {
      if (error) {
        return callback(error);
      }

      nacl.run(nexe, null, callback);
    });
  });
}

describe('C Generator Tests', function() {
  it('should collect tests from test/c/test_gen/data', function(done) {
    var testDataDir = path.resolve(__dirname, 'data');
    fs.readdir(testDataDir, function(error, files) {
      if (error) {
        return done(error);
      }

      var matchesTest = function(f) { return /^test_.*\.cc$/.test(f); },
          tests = Array.prototype.filter.call(files, matchesTest);

      if (tests.length === 0) {
        return done();
      }

      describe('C Generator Tests', function() {
        this.slow(5000);
        this.timeout(30000);

        tests.forEach(function(testName) {
          var baseName = testName.match(/^test_(.*)\.cc$/)[1],
              header = path.join(testDataDir, baseName + '.h'),
              source = path.join(testDataDir, baseName + '.c'),
              testSource = path.join(testDataDir, testName);

          it('should succeed for ' + testName, function(done) {
            genAndRun(header, source, testSource, done);
          });
        });
      });

      done();
    });
  });
});
