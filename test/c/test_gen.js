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

var chai = require('chai');
var assert = chai.assert;
var fs = require('fs');
var gen = require('naclbind-gen');
var nacl = require('nacl-sdk');
var path = require('path');

var toolchain = 'newlib';
var config = 'Debug';
var arch = 'x86_64';

chai.config.includeStack = true;

function genAndRun(header, source, testSource, extraGenOpts, callback) {
  var basename = header.match(/([^.]*)\.h/)[1];
  var outdir = path.resolve(__dirname, '../../out/test/c/test_gen', basename);
  var glueC = path.join(outdir, 'glue.c');
  var glueH = path.join(outdir, 'glue.h');
  var infiles = [
        path.join(__dirname, 'data', source),
        path.join(__dirname, 'data', testSource),
        path.resolve(__dirname, 'test_gen.cc'),
        path.resolve(__dirname, 'fake_interfaces.c'),
        path.resolve(__dirname, 'json.cc'),
        path.resolve(__dirname, 'main.cc')
      ];
  var opts = {
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
            path.resolve(__dirname, '../../src/c'),
            outdir
          ],
          libs: [
            'jsoncpp', 'ppapi_simple', 'gtest', 'nacl_io', 'ppapi_cpp', 'ppapi'
          ]
        },
        translate: {}
      };
  var genOpts = {
        template: ['glue.c', 'glue.h'],
        toolchain: toolchain
      };

  if (arguments.length === 4) {
    callback = extraGenOpts;
    extraOpts = {};
  }

  for (opt in extraGenOpts) {
    genOpts[opt] = extraGenOpts[opt];
  }

  header = path.join(__dirname, 'data', header);

  gen.file(header, [glueC, glueH], genOpts, function(error, outfile) {
    if (error) {
      return callback(error);
    }

    infiles.push(glueC);

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

  it('should succeed for test_typedef', function(done) {
    genAndRun('typedef.h', 'typedef.c', 'test_typedef.cc', done);
  });

  it('should succeed for test_error_if', function(done) {
    genAndRun('error_if.h', 'error_if.c', 'test_error_if.cc', done);
  });

  it('should succeed for test_enum', function(done) {
    genAndRun('enum.h', 'enum.c', 'test_enum.cc', done);
  });

  it('should succeed for test_builtins', function(done) {
    var genOpts = {genArgs: '--builtins'};
    genAndRun('builtins.h', 'builtins.c', 'test_builtins.cc', genOpts, done);
  });

  it('should succeed for test_function_pointers', function(done) {
    genAndRun('function_pointers.h', 'function_pointers.c',
              'test_function_pointers.cc', done);
  });

  it('should succeed for test_callback', function(done) {
    genAndRun('callback.h', 'callback.c', 'test_callback.cc', done);
  });
});
