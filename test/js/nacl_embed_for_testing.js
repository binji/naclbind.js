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

var assert = require('assert');

function NaClEmbedForTesting() {
  if (!(this instanceof NaClEmbedForTesting)) {
    return new NaClEmbedForTesting();
  }
  this.loaded = false;
  this.listeners = {};
  this.postMessageCallback = null;
  this.lastError = undefined;
  this.exitStatus = undefined;
}

NaClEmbedForTesting.prototype.fireEvent = function(message, e) {
  var callbacks = this.listeners[message];
  if (!callbacks) {
    return;
  }

  // Run on the next tick to more closely emulate a real embed.
  process.nextTick(function() {
    for (var i = 0; i < callbacks.length; ++i) {
      callbacks[i](e);
    }
  });
};

NaClEmbedForTesting.prototype.addEventListener_ = function(message, callback) {
  if (!this.listeners[message]) {
    this.listeners[message] = [];
  }

  this.listeners[message].push(callback);
};

NaClEmbedForTesting.prototype.load = function() {
  this.loaded = true;
  this.fireEvent('load', null);
};

NaClEmbedForTesting.prototype.message = function(msg) {
  var event = {data: msg};
  this.fireEvent('message', event);
};

NaClEmbedForTesting.prototype.error = function(error) {
  this.lastError = error;
  this.fireEvent('error', null);
};

NaClEmbedForTesting.prototype.exit = function(exitStatus) {
  this.exitStatus = exitStatus;
  this.fireEvent('crash', null);
};

NaClEmbedForTesting.prototype.crash = function() {
  this.exit(-1);
};

NaClEmbedForTesting.prototype.addLoadListener = function(callback) {
  this.addEventListener_('load', callback);
};

NaClEmbedForTesting.prototype.addMessageListener = function(callback) {
  this.addEventListener_('message', callback);
};

NaClEmbedForTesting.prototype.addErrorListener = function(callback) {
  this.addEventListener_('error', callback);
};

NaClEmbedForTesting.prototype.addCrashListener = function(callback) {
  this.addEventListener_('crash', callback);
};

NaClEmbedForTesting.prototype.appendToBody = function() {
};

NaClEmbedForTesting.prototype.setPostMessageCallback = function(callback) {
  this.postMessageCallback = callback;
}

NaClEmbedForTesting.prototype.postMessage = function(msg) {
  assert(this.postMessageCallback);
  this.postMessageCallback(msg);
};

module.exports = NaClEmbedForTesting;
