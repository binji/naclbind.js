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
  var repo;
  var sig;
  var index;
  var tree_id, commit_id;
  var tree;

  function dostuff() {
    promise.resolve().then(function() {
      f.git_threads_init(c);
      repo = m.mallocType(c, t.git_repository$);
      var result = f.git_repository_init(c, repo, dirname, 0);
      return m.commitPromise(result);
    }).then(function(result) {
      console.log('git_repository_init returned ' + result);

      repo = f.get(c, repo).cast(t.git_repository$);

      sig = m.mallocType(c, t.git_signature$);
      var result = f.git_signature_default(c, sig, repo);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result < 0) {
        var err = f.giterr_last(c);
        var message = m.getField(c, t._git_error.fields.message, err);
        f.puts(c, message);
        return m.commitPromise();
      }
    }).catch(function(err) {
      console.error('error: ' + err.stack);
    }).finally(function() {
      f.git_repository_free(c, repo);
      f.git_threads_shutdown(c);
      m.destroyHandles(c);
      return m.commitPromise();
    });
  }

  dostuff();
});
