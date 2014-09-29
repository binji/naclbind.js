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
    child_process = require('child_process'),
    fs = require('fs'),
    gen = require('../shared/gen'),
    path = require('path'),
    Promise = require('bluebird'),
    execFile = child_process.execFile,

    toolchain = 'newlib',
    arch = 'x86_64',
    config = 'Debug',

    tools = ['c++', 'translate', 'finalize'],
    toolsHash = null;

function prefixAll(prefix, a) {
  return a.map(function(el) { return prefix + el; });
}

function maybeConcatWithPrefix(args, prefix, a) {
  if (a) {
    return args.concat(prefixAll(prefix, a));
  }

  return args;
}

function getDefaultIncludeDirs() {
  var sdkRoot = process.env.NACL_SDK_ROOT;
  return [path.join(sdkRoot, 'include'),
          path.join(sdkRoot, 'include', toolchain)];
}

function getDefaultLibDirs() {
  var sdkRoot = process.env.NACL_SDK_ROOT;
  if (toolchain === 'pnacl') {
    return [path.join(sdkRoot, 'lib', toolchain, config)];
  } else {
    return [path.join(sdkRoot, 'lib', toolchain + '_' + arch, config)];
  }
}

function execFilePromise(cmd, args, opts) {
  return new Promise(function(resolve, reject) {
    execFile(cmd, args, opts, function(error, stdout, stderr) {
      if (error) {
        return reject(error);
      }

      return resolve(stdout);
    });
  });
}

function runNaclConfigForTool(naclConfig, tool) {
  var args = [naclConfig, '-t', toolchain];
  if (toolchain !== 'pnacl') {
    args.push('-a');
    args.push(arch);
  }

  args.push('--tool');
  args.push(tool);

  return execFilePromise('python', args).then(function(stdout) {
    return stdout.trim();
  });
}

function runNaclConfigMulti(naclConfig, tools) {
  var promises = tools.map(function(t) {
        return runNaclConfigForTool(naclConfig, t);
      });

  return Promise.all(promises).then(function(paths) {
    var result = {},
        i;
    for (i = 0; i < tools.length; ++i) {
      result[tools[i]] = paths[i];
    }

    return result;
  });
}

function compile(infiles, outfile, opts) {
  var args = [],
      lastExt = null,
      execOpts = {maxBuffer: 500 * 1024},
      i,
      ext;

  for (i = 0; i < infiles.length; ++i) {
    ext = path.extname(infiles[i]);
    if (ext !== lastExt) {
      if (ext === '.c') {
        args.push('-x');
        args.push('c');
      } else if (ext === '.cc') {
        args.push('-x');
        args.push('c++');
      } else {
        throw new Error('Unknown extension ' + ext);
      }
    }

    args.push(infiles[i]);
  };
  args.push('-o');
  args.push(outfile);

  opts = opts || {};

  // Add default include directories.
  opts.includeDirs = opts.includeDirs || [];
  args = maybeConcatWithPrefix(args, '-I', getDefaultIncludeDirs());
  args = maybeConcatWithPrefix(args, '-L', getDefaultLibDirs());

  args = maybeConcatWithPrefix(args, '-D', opts.defines);
  args = maybeConcatWithPrefix(args, '-I', opts.includeDirs);
  args = maybeConcatWithPrefix(args, '-L', opts.libDirs);
  args = maybeConcatWithPrefix(args, '-l', opts.libs);

  if (opts.args) {
    args = args.concat(opts.args);
  }

  return execFilePromise(toolsHash['c++'], args, execOpts).then(function() {
    return outfile;
  });
}

function finalize(infile, outfile, opts) {
  var args = [infile, '-o', outfile];
  opts = opts || {};

  if (opts.compress) {
    args.push('--compress');
  }

  return execFilePromise(toolsHash.finalize, args).then(function() {
    return outfile;
  });
}

function translate(infile, outfile, opts) {
  var args = [infile, '-o', outfile];
  opts = opts || {};

  if (opts.arch) {
    args.push('-arch');
    args.push(opts.arch);
  }

  if (opts.opt) {
    args.push('-O' + opts.opt);
  }

  return execFilePromise(toolsHash.translate, args).then(function() {
    return outfile;
  });
}

function genFile(infile, opts, callback) {
  gen.tmpDir(function(error, dirname) {
    if (error) {
      return callback(error);
    }

    var outfile = path.join(dirname, 'gen.c'),
        p;

    p = gen.filePromise(infile, outfile, 'glue.c').then(function(genfile) {
      var compileOpts = opts.compile || {},
          infiles = (compileOpts.infiles || []).slice(),
          outname = toolchain === 'pnacl' ? 'gen.bc' : 'gen.nexe';
          outfile = path.join(dirname, outname);
      infiles.push(genfile);

      return compile(infiles, outfile, opts.compile);
    });

    if (toolchain === 'pnacl') {
      p = p.then(function(bc) {
        var finalizeOpts = opts.finalize || {},
            outfile = path.join(dirname, 'gen.pexe');
        return finalize(bc, outfile, finalizeOpts);
      }).then(function(pexe) {
        var translateOpts = opts.translate || {},
            outfile = path.join(dirname, 'gen.nexe');
        return translate(pexe, outfile, translateOpts);
      });
    }

    return p.then(function(nexe) {
      callback(null, nexe);
    }).catch(function(error) {
      callback(error);
    });
  });
}

function run(nexe, args, callback) {
  var selLdr = path.join(process.env.NACL_SDK_ROOT, 'tools', 'sel_ldr.py');
  args = [selLdr, nexe, '--'].concat(args || []);
  execFile('python', args, function(error, stdout, stderr) {
    if (error) {
      callback(new Error(error.toString() + '\n' + stdout));
      return;
    }

    // console.log('STDOUT\n', stdout, 'STDERR\n', stderr);
    callback(null);
  });
}

function genAndRun(header, source, testSource, callback) {
  var opts = {
    compile: {
      infiles: [
        source,
        testSource,
        path.resolve(__dirname, 'test_gen.cc'),
        path.resolve(__dirname, 'fake_interfaces.c'),
        path.resolve(__dirname, 'json.cc'),
        path.resolve(__dirname, 'main.cc')
      ],
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
    translate: {
      arch: arch
    }
  };

  genFile(header, opts, function(error, nexe) {
    if (error) {
      return callback(error);
    }

    run(nexe, null, callback);
  });
}

describe('Collect C Generator Tests', function() {
  it('should succeed', function(done) {
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

      describe('Generate C', function() {
        this.slow(5000);
        this.timeout(30000);

        before(function() {
          if (!process.env.NACL_SDK_ROOT) {
            assert.ok(false, 'NACL_SDK_ROOT not set.');
          }

          var toolsDir = path.resolve(process.env.NACL_SDK_ROOT, 'tools'),

          naclConfig = path.join(toolsDir, 'nacl_config.py');

          return runNaclConfigMulti(naclConfig, tools).then(function(hash) {
            toolsHash = hash;
          }).catch(function(error) {
            assert.ok(false, 'Failed to run nacl_config:\n' + error);
          });
        });

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
