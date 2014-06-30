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

"use strict";

require(['promise', 'nacl', 'git_glue'], function(promise, nacl, git_glue) {
  var m = git_glue;
  var t = m.types;
  var f = m.functions;

  var c = m.makeContext();
  var dirname = '/';

  function dostuff() {
    promise.resolve().then(function() {
      f.git_threads_init(c);
      var lenPtr = m.mallocType(c, t.uint32);
      var dirnameCstr = f.varToUtf8(c, dirname, lenPtr);
      var repo = m.mallocType(c, t.git_repository$);
      var result = f.git_repository_init(c, repo, dirnameCstr, 0);
      return m.commitPromise(result);
    }).then(function(result) {
      console.log('git_repository_init returned ' + result);
    }).catch(function(err) {
      console.error('error: ' + err.stack);
    });
  }

  dostuff();
});
