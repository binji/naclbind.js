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

require(['promise', 'git_glue'], function(promise, git_glue) {
  var m = git_glue;
  var t = m.types;

  var c = m.makeContext();
  var dirname = '/';
  var repo;
  var sig;
  var index;
  var tree_id, commit_id;
  var tree;

  function writeError() {
    var err = c.giterr_last();
    var message = c.$getField(t._git_error.fields.message, err);
    c.puts(message);
    return m.commitPromise();
  }

  function dostuff() {
    promise.resolve().then(function() {
      c.git_threads_init();
      repo = c.$mallocType(t.git_repository$);
      var result = c.git_repository_init(repo, dirname, 0);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result < 0) {
        return writeError();
      }

      repo = c.get(repo).cast(t.git_repository$);
      repo.setFinalizer(c.git_repository_free.bind(c, repo));

      sig = c.$mallocType(t.git_signature$);
      var result = c.git_signature_now(sig, "Foo Bar", "foobar@example.com");
      return m.commitPromise(result);
    }).then(function(result) {
      if (result < 0) {
        return writeError();
      }

      sig = c.get(sig).cast(t.git_signature$);
      sig.setFinalizer(c.git_signature_free.bind(c, sig));

      index = c.$mallocType(t.git_index$);
      var result = c.git_repository_index(index, repo);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result < 0) {
        return writeError();
      }

      index = c.get(index).cast(t.git_index$);
      index.setFinalizer(c.git_index_free.bind(c, index));

      tree_id = c.$mallocType(t.git_oid);
      var result = c.git_index_write_tree(tree_id, index);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result < 0) {
        return writeError();
      }

      tree = c.$mallocType(t.git_tree$);
      var result = c.git_tree_lookup(tree, repo, tree_id);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result < 0) {
        return writeError();
      }

      tree = c.get(tree).cast(t.git_tree$);
      tree.setFinalizer(c.git_tree_free.bind(c, tree));

      commit_id = c.$mallocType(t.git_oid);
      var result = c.git_commit_create(commit_id, repo, "HEAD", sig, sig,
                                       null, "Initial Commit", tree, 0, null);

      return m.commitPromise(result);
    }).catch(function(err) {
      console.error('error: ' + err.stack);
    }).finally(function() {
      c.git_threads_shutdown();
      c.$destroyHandles();
      return m.commitPromise();
    });
  }

  dostuff();
});
