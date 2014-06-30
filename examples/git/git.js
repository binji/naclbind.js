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

  function writeError() {
    var err = f.giterr_last(c);
    var message = m.getField(c, t._git_error.fields.message, err);
    f.puts(c, message);
    return m.commitPromise();
  }

  function dostuff() {
    promise.resolve().then(function() {
      f.git_threads_init(c);
      repo = m.mallocType(c, t.git_repository$);
      var result = f.git_repository_init(c, repo, dirname, 0);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result < 0) {
        return writeError();
      }

      repo = f.get(c, repo).cast(t.git_repository$);

      sig = m.mallocType(c, t.git_signature$);
      var result = f.git_signature_now(c, sig, "Foo Bar", "foobar@example.com");
      return m.commitPromise(result);
    }).then(function(result) {
      if (result < 0) {
        return writeError();
      }

      sig = f.get(c, sig).cast(t.git_signature$);

      index = m.mallocType(c, t.git_index$);
      var result = f.git_repository_index(c, index, repo);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result < 0) {
        return writeError();
      }

      index = f.get(c, index).cast(t.git_index$);
      tree_id = m.mallocType(c, t.git_oid);
      var result = f.git_index_write_tree(c, tree_id, index);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result < 0) {
        return writeError();
      }

      f.git_index_free(c, index);
    }).then(function(result) {
      if (result < 0) {
        return writeError();
      }

      tree = m.mallocType(c, t.git_tree$);
      var result = f.git_tree_lookup(c, tree, repo, tree_id);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result < 0) {
        return writeError();
      }

      tree = f.get(c, tree).cast(t.git_tree$);

      commit_id = m.mallocType(c, t.git_oid);
      var result = f.git_commit_create(c, commit_id, repo, "HEAD", sig, sig,
                                       null, "Initial Commit", tree, 0, null);

      return m.commitPromise(result);
    }).then(function(result) {
      if (result < 0) {
        return writeError();
      }

      f.git_tree_free(c, tree);
      f.git_signature_free(c, sig);
      return m.commitPromise();
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
