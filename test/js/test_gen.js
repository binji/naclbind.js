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
    fs = require('fs'),
    path = require('path'),
    tmp = require('tmp'),
    type = require('../../src/js/type'),
    utils = require('../../src/js/utils'),
    execFile = child_process.execFile;

function appendPath(s, path) {
  return s.split(':').concat([path]).join(':');
}

function genText(text, callback) {
  var oldCallback = callback;

  tmp.file(function(error, path, fd, cleanup) {
    fs.writeFile(path, text, function(error) {
      if (error) {
        return callback(error);
      }

      genFile(path, function(error, m) {
        if (error) {
          return callback(error);
        }

        callback(null, m);
      });
    });
  });
}

function assertTypesEqual(t1, t2) {
  assert.ok(t1.equals(t2));
}

function assertFieldsEqual(f, name, type, offset) {
  assert.strictEqual(name, f.name);
  assertTypesEqual(type, f.type);
  assert.strictEqual(offset, f.offset);
}

function genFile(inpath, callback) {
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

    var outpath = path.join(outdir, 'gen.js'),
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

describe('Generate JS', function() {
  this.slow(800);

  before(function() {
    // Hack NODE_PATH to include the source directory automatically.
    var srcDir = path.resolve(__dirname, '../../src/js'),
        nodePath = appendPath(process.env.NODE_PATH || '', srcDir);
    process.env.NODE_PATH = nodePath;
    require('module')._initPaths();

    if (!process.env.NACL_SDK_ROOT) {
      assert.ok(false, 'NACL_SDK_ROOT not set.');
    }
  });

  it('should do work for a simple function', function(done) {
    genText('void foo(void);\n', function(error, m) {
      if (error) {
        assert.ok(false, 'Error generating JS.\n' + error);
      }

      var expected = type.Function(type.void, []);

      assert.strictEqual(1, m.$functionsCount);
      assert.ok(m.foo);
      assert.strictEqual(1, m.foo.types.length);
      assert.ok(expected.equals(m.foo.types[0]));
      assert.strictEqual(0, m.$typesCount);
      assert.strictEqual(0, m.$tagsCount);
      done();
    });
  });

  it('should generate struct types', function(done) {
    genFile('../data/structs.h', function(error, m) {
      if (error) {
        assert.ok(false, 'Error generating JS.\n' + error);
      }

      assert.strictEqual(8, m.$functionsCount);

      // Incomplete struct == s1
      assert.ok(m.$tags.s1);
      assert.strictEqual('s1', m.$tags.s1.tag);
      assert.strictEqual(-2, m.$tags.s1.size);
      assert.strictEqual(0, m.$tags.s1.fields.length);
      assert.strictEqual(false, m.$tags.s1.isUnion);

      // Empty struct == s2
      assert.ok(m.$tags.s2);
      assert.strictEqual('s2', m.$tags.s2.tag);
      assert.strictEqual(0, m.$tags.s2.size);
      assert.strictEqual(0, m.$tags.s2.fields.length);
      assert.strictEqual(false, m.$tags.s2.isUnion);

      // Basic struct == s3
      assert.ok(m.$tags.s3);
      assert.strictEqual('s3', m.$tags.s3.tag);
      assert.strictEqual(4, m.$tags.s3.size);
      assert.strictEqual(1, m.$tags.s3.fields.length);
      assertFieldsEqual(m.$tags.s3.fields[0], 'f', type.int, 0);
      assert.strictEqual(false, m.$tags.s3.isUnion);

      // Struct with typedef == s4, t1
      assert.ok(m.$tags.s4);
      assert.strictEqual('s4', m.$tags.s4.tag);
      assert.strictEqual(4, m.$tags.s4.size);
      assert.strictEqual(1, m.$tags.s4.fields.length);
      assertFieldsEqual(m.$tags.s4.fields[0], 'g', type.int, 0);
      assert.strictEqual(false, m.$tags.s4.isUnion);

      assert.ok(m.$types.t1);
      assert.strictEqual('t1', m.$types.t1.tag);
      assertTypesEqual(m.$tags.s4, m.$types.t1.alias);
      assert.strictEqual('t1', m.$types.t1.spelling);
      assert.strictEqual();

      // Anonymous nested struct, named field == s5
      assert.ok(m.$tags.s5);
      assert.strictEqual('s5', m.$tags.s5.tag);
      assert.strictEqual(8, m.$tags.s5.size);
      assert.strictEqual(1, m.$tags.s5.fields.length);
      assert.strictEqual('nested', m.$tags.s5.fields[0].name);
      assert.strictEqual(0, m.$tags.s5.fields[0].offset);
      assert.strictEqual(false, m.$tags.s5.isUnion);

      assert.strictEqual(2, m.$tags.s5.fields[0].type.fields.length);
      assertFieldsEqual(m.$tags.s5.fields[0].type.fields[0], 'f', type.int, 0);
      assertFieldsEqual(m.$tags.s5.fields[0].type.fields[1], 'g', type.int, 4);

      // Anonymous nested struct, unnamed field == s6
      assert.ok(m.$tags.s6);
      assert.strictEqual('s6', m.$tags.s6.tag);
      assert.strictEqual(8, m.$tags.s6.size);
      assert.strictEqual(2, m.$tags.s6.fields.length);
      assertFieldsEqual(m.$tags.s6.fields[0], 'f', type.int, 0);
      assertFieldsEqual(m.$tags.s6.fields[1], 'g', type.int, 4);
      assert.strictEqual(false, m.$tags.s6.isUnion);

      // Named nested struct, named field == s7
      assert.ok(m.$tags.s7);
      assert.strictEqual('s7', m.$tags.s7.tag);
      assert.strictEqual(8, m.$tags.s7.size);
      assert.strictEqual(2, m.$tags.s7.fields.length);
      assertFieldsEqual(m.$tags.s7.fields[0], 'g', m.$tags.ns1, 0);
      assertFieldsEqual(m.$tags.s7.fields[1], 'h', type.int, 4);
      assert.strictEqual(false, m.$tags.s7.isUnion);

      assert.ok(m.$tags.ns1);
      assert.strictEqual('ns1', m.$tags.ns1.tag);
      assert.strictEqual(4, m.$tags.ns1.size);
      assert.strictEqual(1, m.$tags.ns1.fields.length);
      assertFieldsEqual(m.$tags.ns1.fields[0], 'f', type.int, 0);

      // Named nested struct, unnamed field == s8
      assert.ok(m.$tags.s8);
      assert.strictEqual('s8', m.$tags.s8.tag);
      assert.strictEqual(4, m.$tags.s8.size);
      assert.strictEqual(1, m.$tags.s8.fields.length);
      assertFieldsEqual(m.$tags.s8.fields[0], 'g', type.int, 0);
      assert.strictEqual(false, m.$tags.s8.isUnion);

      done();
    });
  });

  it('should generate union types', function(done) {
    genFile('../data/unions.h', function(error, m) {
      if (error) {
        assert.ok(false, 'Error generating JS.\n' + error);
      }

      assert.strictEqual(8, m.$functionsCount);

      // Incomplete union == s1
      assert.ok(m.$tags.s1);
      assert.strictEqual('s1', m.$tags.s1.tag);
      assert.strictEqual(-2, m.$tags.s1.size);
      assert.strictEqual(0, m.$tags.s1.fields.length);
      assert.strictEqual(true, m.$tags.s1.isUnion);

      // Empty union == s2
      assert.ok(m.$tags.s2);
      assert.strictEqual('s2', m.$tags.s2.tag);
      assert.strictEqual(0, m.$tags.s2.size);
      assert.strictEqual(0, m.$tags.s2.fields.length);
      assert.strictEqual(true, m.$tags.s2.isUnion);

      // Basic union == s3
      assert.ok(m.$tags.s3);
      assert.strictEqual('s3', m.$tags.s3.tag);
      assert.strictEqual(4, m.$tags.s3.size);
      assert.strictEqual(1, m.$tags.s3.fields.length);
      assertFieldsEqual(m.$tags.s3.fields[0], 'f', type.int, 0);
      assert.strictEqual(true, m.$tags.s3.isUnion);

      // Union with typedef == s4, t1
      assert.ok(m.$tags.s4);
      assert.strictEqual('s4', m.$tags.s4.tag);
      assert.strictEqual(4, m.$tags.s4.size);
      assert.strictEqual(1, m.$tags.s4.fields.length);
      assertFieldsEqual(m.$tags.s4.fields[0], 'g', type.int, 0);
      assert.strictEqual(true, m.$tags.s4.isUnion);

      assert.ok(m.$types.t1);
      assert.strictEqual('t1', m.$types.t1.tag);
      assertTypesEqual(m.$tags.s4, m.$types.t1.alias);
      assert.strictEqual('t1', m.$types.t1.spelling);
      assert.strictEqual();

      // Anonymous nested union, named field == s5
      assert.ok(m.$tags.s5);
      assert.strictEqual('s5', m.$tags.s5.tag);
      assert.strictEqual(4, m.$tags.s5.size);
      assert.strictEqual(1, m.$tags.s5.fields.length);
      assert.strictEqual('nested', m.$tags.s5.fields[0].name);
      assert.strictEqual(0, m.$tags.s5.fields[0].offset);
      assert.strictEqual(true, m.$tags.s5.isUnion);

      assert.strictEqual(2, m.$tags.s5.fields[0].type.fields.length);
      assertFieldsEqual(m.$tags.s5.fields[0].type.fields[0], 'f', type.int, 0);
      assertFieldsEqual(m.$tags.s5.fields[0].type.fields[1], 'g', type.int, 0);

      // Anonymous nested union, unnamed field == s6
      assert.ok(m.$tags.s6);
      assert.strictEqual('s6', m.$tags.s6.tag);
      assert.strictEqual(4, m.$tags.s6.size);
      assert.strictEqual(2, m.$tags.s6.fields.length);
      assertFieldsEqual(m.$tags.s6.fields[0], 'f', type.int, 0);
      assertFieldsEqual(m.$tags.s6.fields[1], 'g', type.int, 0);
      assert.strictEqual(true, m.$tags.s6.isUnion);

      // Named nested union, named field == s7
      assert.ok(m.$tags.s7);
      assert.strictEqual('s7', m.$tags.s7.tag);
      assert.strictEqual(4, m.$tags.s7.size);
      assert.strictEqual(2, m.$tags.s7.fields.length);
      assertFieldsEqual(m.$tags.s7.fields[0], 'g', m.$tags.ns1, 0);
      assertFieldsEqual(m.$tags.s7.fields[1], 'h', type.int, 0);
      assert.strictEqual(true, m.$tags.s7.isUnion);

      assert.ok(m.$tags.ns1);
      assert.strictEqual('ns1', m.$tags.ns1.tag);
      assert.strictEqual(4, m.$tags.ns1.size);
      assert.strictEqual(1, m.$tags.ns1.fields.length);
      assertFieldsEqual(m.$tags.ns1.fields[0], 'f', type.int, 0);

      // Named nested union, unnamed field == s8
      assert.ok(m.$tags.s8);
      assert.strictEqual('s8', m.$tags.s8.tag);
      assert.strictEqual(4, m.$tags.s8.size);
      assert.strictEqual(1, m.$tags.s8.fields.length);
      assertFieldsEqual(m.$tags.s8.fields[0], 'g', type.int, 0);
      assert.strictEqual(true, m.$tags.s8.isUnion);

      done();
    });
  });
});
