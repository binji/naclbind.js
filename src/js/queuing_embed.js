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

var utils = require('./utils');

function QueuingEmbed(embed) {
  if (!(this instanceof QueuingEmbed)) {
    return new QueuingEmbed(embed);
  }
  this.embed_ = embed;

  this.queuedMessages_ = [];
  this.embed_.addLoadListener(this.onLoad_.bind(this));
  this.embed_.addMessageListener(this.onMessage_.bind(this));
  this.loaded_ = false;

  this.nextId_ = 1;
  this.idCallbackMap_ = [];
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

QueuingEmbed.prototype.onMessage_ = function(e) {
  var msg = e.data,
      jsonMsg,
      id,
      callback;

  if (typeof(msg) !== 'object') {
    jsonMsg = JSON.stringify(msg);
    throw new Error('Unexpected value from module: ' + jsonMsg);
  }

  id = msg.id;
  if (!(utils.isNumber(id) && utils.isInteger(id))) {
    jsonMsg = JSON.stringify(msg);
    throw new Error('Received message with bad id: ' + jsonMsg);
  }

  callback = this.idCallbackMap_[id];
  if (utils.getClass(callback) !== 'Function') {
    jsonMsg = JSON.stringify(msg);
    throw new Error('No callback associated with id: ' + id + ' for msg: ' +
                    jsonMsg);
  }

  callback(msg);
  delete this.idCallbackMap_[id];
};

QueuingEmbed.prototype.postQueuedMessages_ = function() {
  var i;
  for (i = 0; i < this.queuedMessages_.length; ++i) {
    this.embed_.postMessage(this.queuedMessages_[i]);
  }
  this.queuedMessages_ = null;
};

QueuingEmbed.prototype.postMessage = function(msg, callback) {
  var id = this.nextId_++;

  this.idCallbackMap_[id] = callback;
  msg.id = id;

  if (!this.loaded_) {
    this.queuedMessages_.push(msg);
    return;
  }

  this.embed_.postMessage(msg);
};

QueuingEmbed.prototype.appendToBody = function() {
  this.embed_.appendToBody();
};

module.exports = QueuingEmbed;
