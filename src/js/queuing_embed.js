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

function QueuingEmbed(embed) {
  if (!(this instanceof QueuingEmbed)) {
    return new QueuingEmbed(embed);
  }
  this.embed_ = embed;

  this.queuedMessages_ = [];
  this.embed_.addLoadListener(this.onLoad_.bind(this));
  this.loaded_ = false;
}

QueuingEmbed.prototype.onLoad_ = function(e) {
  // Wait till the next time through the eventloop to allow other 'load'
  // listeners to be called.
  var self = this;
  process.nextTick(function() {
    self.postQueuedMessages_();
  });

  this.loaded_ = true;
};

QueuingEmbed.prototype.postQueuedMessages_ = function() {
  var i;
  for (i = 0; i < this.queuedMessages_.length; ++i) {
    this.embed_.postMessage(this.queuedMessages_[i]);
  }
  this.queuedMessages_ = null;
};

QueuingEmbed.prototype.addLoadListener = function(callback) {
  this.embed_.addLoadListener(callback);
};

QueuingEmbed.prototype.addMessageListener = function(callback) {
  this.embed_.addMessageListener(callback);
};

QueuingEmbed.prototype.addErrorListener = function(callback) {
  this.embed_.addErrorListener(callback);
};

QueuingEmbed.prototype.addCrashListener = function(callback) {
  this.embed_.addCrashListener(callback);
};

QueuingEmbed.prototype.appendToBody = function() {
  this.embed_.appendToBody();
};

QueuingEmbed.prototype.postMessage = function(msg) {
  if (!this.loaded_) {
    this.queuedMessages_.push(msg);
    return;
  }

  this.embed_.postMessage(msg);
};

Object.defineProperty(QueuingEmbed.prototype, 'lastError', {
  get: function() { return this.element.lastError; },
  enumerable: true
});

Object.defineProperty(QueuingEmbed.prototype, 'exitStatus', {
  get: function() { return this.element.exitStatus; },
  enumerable: true
});

module.exports = QueuingEmbed;
