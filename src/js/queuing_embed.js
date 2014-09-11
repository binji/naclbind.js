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

  this.delegateTo_(this.embed_);
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

QueuingEmbed.prototype.delegateTo_ = function(that) {
  var p,
      proto,
      desc;

  for (p in this.embed_) {
    // If this name is already defined, skip.
    if (p in this) {
      continue;
    }

    // Delegate functions.
    if (typeof that[p] === 'function') {
      this[p] = makeFunctionDelegate(that, p);
      continue;
    }

    // Delegate non-function properties.
    proto = that;
    while (proto && !proto.hasOwnProperty(p)) {
      proto = Object.getPrototypeOf(proto);
    }

    if (!proto) {
      continue;
    }

    desc = Object.getOwnPropertyDescriptor(proto, p);
    delete desc.value;
    desc.get = makeGetDelegate(that, p);
    if (desc.writable) {
      delete desc.writable;
      desc.set = makeSetDelegate(that, p);
    }
    Object.defineProperty(this, p, desc);
  }
};

function makeFunctionDelegate(embed, propertyName) {
  return function() { return embed[propertyName].apply(embed, arguments); };
}

function makeGetDelegate(embed, propertyName) {
  return function() { return embed[propertyName]; };
}

function makeSetDelegate(embed, propertyName) {
  return function(value) { embed[propertyName] = value; };
}

QueuingEmbed.prototype.postMessage = function(msg) {
  if (!this.loaded_) {
    this.queuedMessages_.push(msg);
    return;
  }

  this.embed_.postMessage(msg);
};

module.exports = QueuingEmbed;
