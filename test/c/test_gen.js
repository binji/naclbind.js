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
    fs = require('fs'),
    gen = require('naclbind-gen'),
    nacl = require('nacl-sdk'),
    path = require('path'),

    toolchain = 'newlib',
    config = 'Debug',
    arch = 'x86_64';

chai.config.includeStack = true;

function genAndRun(header, source, testSource, callback) {
  var basename = header.match(/([^.]*)\.h/)[1],
      outdir = path.resolve(__dirname, '../../out/test/c/test_gen', basename),
      glueC = path.join(outdir, 'glue.c'),
      infiles = [
        path.join(__dirname, 'data', source),
        path.join(__dirname, 'data', testSource),
        path.resolve(__dirname, 'test_gen.cc'),
        path.resolve(__dirname, 'fake_interfaces.c'),
        path.resolve(__dirname, 'json.cc'),
        path.resolve(__dirname, 'main.cc')
      ],
      opts = {
        toolchain: toolchain,
        config: config,
        arch: arch,
        outdir: outdir,

        compile: {
          args: ['-Wall', '-Werror', '-pthread'],
          defines: [
            'NB_NO_APP',
            'restrict='  // Ignore restrict; it doesn't compile with nacl-gcc
          ],
          includeDirs: [
            path.resolve(__dirname),
            path.resolve(__dirname, '..'),
            path.resolve(__dirname, '../../src/c')
          ],
          libs: [
            'jsoncpp', 'ppapi_simple', 'gtest', 'nacl_io', 'ppapi_cpp', 'ppapi'
          ]
        },
        translate: {}
      },
      genOpts = {
        template: 'glue.c',
        toolchain: toolchain
      }

  header = path.join(__dirname, 'data', header);

  gen.file(header, glueC, genOpts, function(error, outfile) {
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
  this.slow(5000);
  this.timeout(30000);

  it('should succeed for test_multi', function(done) {
    genAndRun('multi.h', 'multi.c', 'test_multi.cc', done);
  });

  it('should succeed for test_noproto', function(done) {
    genAndRun('noproto.h', 'noproto.c', 'test_noproto.cc', done);
  });

  it('should succeed for test_primitives', function(done) {
    genAndRun('primitives.h', 'primitives.c', 'test_primitives.cc', done);
  });

  it('should succeed for test_simple', function(done) {
    genAndRun('simple.h', 'simple.c', 'test_simple.cc', done);
  });

  it('should succeed for test_struct', function(done) {
    genAndRun('struct.h', 'struct.c', 'test_struct.cc', done);
  });

  it('should succeed for test_restrict', function(done) {
    genAndRun('restrict.h', 'restrict.c', 'test_restrict.cc', done);
  });

  it('should succeed for test_alignment', function(done) {
    genAndRun('alignment.h', 'alignment.c', 'test_alignment.cc', done);
  });

  it('should succeed for test_variadic', function(done) {
    genAndRun('variadic.h', 'variadic.c', 'test_variadic.cc', done);
  });

  it('should succeed for test_const', function(done) {
    genAndRun('const.h', 'const.c', 'test_const.cc', done);
  });
});
