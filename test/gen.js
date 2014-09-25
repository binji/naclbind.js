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

var Promise = require('bluebird');

var child_process = require('child_process'),
    fs = require('fs'),
    path = require('path'),
    tmp = require('tmp'),
    execFile = child_process.execFile;

function tmpDir(callback) {
  var opts = {prefix: 'gen', unsafeCleanup: true};
  tmp.dir(opts, function(error, outdir) {
    if (error) {
      return callback(error);
    }

    callback(null, outdir);
  });
}

function genFile(infile, outfile, templateName, callback) {
  var genPy = path.resolve(__dirname, '../bin/gen.py'),
      glueJs = path.resolve(__dirname, '../templates', templateName),
      cmd = [genPy, '-t', glueJs, infile, '-o', outfile],
      opts = {cwd: __dirname};

  execFile('python', cmd, opts, function(error, stdout, stderr) {
    if (error) {
      return callback(error);
    }

    callback(null, outfile);
  });
}

module.exports = {
  file: genFile,
  filePromise: Promise.promisify(genFile),
  tmpDir: tmpDir
};
