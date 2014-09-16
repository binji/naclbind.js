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
    execFile = child_process.execFile,
    spawn = child_process.spawn,
    toolchain = 'newlib',

    outDir = path.resolve(__dirname, '../../out/test', toolchain, 'Debug'),
    testNexe = path.join(outDir, 'test_x86_64.nexe');

if (!process.env.NACL_SDK_ROOT) {
  assert.ok(false, 'NACL_SDK_ROOT not set, skipping tests.');
}

// Compiling the test takes a while.
//this.enableTimeouts(false);

function startswith(s, prefix) {
  return s.lastIndexOf(prefix, 0) === 0;
}

function buildTest(callback) {
  var cmd = ['CONFIG=Debug', 'TOOLCHAIN=' + toolchain, 'test'],
      opts = {cwd: __dirname, stdio: 'inherit'},
      proc,
      start = new Date();

  console.log('Making C tests...');
  proc = spawn('make', cmd, opts);

  proc.on('close', function(error) {
    var end = new Date();

    if (error) {
      assert.ok(false, 'Unable to make test:\n' + error);
      return;
    }

    console.log('...done (' + (end - start) + 'ms).\n');

    fs.stat(testNexe, function(error, stat) {
      if (error) {
        assert.ok(false, 'Make succeeded, but cannot stat ' + testNexe);
        return;
      }

      parseTests(callback);
    });
  });
}

function parseTests(callback) {
  runTest(['--gtest_list_tests'], function(error, stdout) {
    var suites = [],
        suite = null;

    if (error) {
      callback(error);
      return;
    }

    function addSuite() {
      if (suite) {
        suites.push(suite);
      }
    }

    // Parse the stdout for a list of tests.
    stdout.split('\n').forEach(function(line) {
      if (startswith(line, '  ')) {
        suite.cases.push(line.trim());
      } else {
        // Add previous suite, if any.
        addSuite();

        // Create a new suite.
        var suiteName = line.trim().slice(0, -1);

        if (suiteName) {
          suite = {name: suiteName, cases: []};
        } else {
          suite = null;
        }
      }
    });

    addSuite();

    describe('C', function() {
      suites.forEach(function(suite) {
        describe(suite.name, function() {
          suite.cases.forEach(function(testCase) {
            it(testCase, function(done) {
              this.slow(150);
              runTest(['--gtest_filter='+suite.name+'.'+testCase], done);
            });
          });
        });
      });
    });

    callback();
  });
}

function runTest(args, callback) {
  var selLdr = path.join(process.env.NACL_SDK_ROOT, 'tools', 'sel_ldr.py');
  args = [selLdr, testNexe, '--'].concat(args || []);
  execFile('python', args, function(error, stdout, stderr) {
    if (error) {
      callback(new Error(error.toString() + '\n' + stdout));
      return;
    }

    callback(null, stdout);
  });
}

before(function(done) {
  buildTest(done);
});
