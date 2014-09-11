var assert = require('assert');

function EmbedForTesting(nmf, mimeType) {
  if (!(this instanceof EmbedForTesting)) {
    return new EmbedForTesting(nmf, mimeType);
  }
  this.loaded = false;
  this.nmf = nmf;
  this.mimeType = mimeType;
  this.listeners = {};
  this.postMessageCallback = null;
  this.lastError = undefined;
  this.exitStatus = undefined;
}

EmbedForTesting.prototype.fireEvent = function(message, e) {
  var callbacks = this.listeners[message];
  if (!callbacks) {
    return;
  }

  for (var i = 0; i < callbacks.length; ++i) {
    callbacks[i](e);
  }
};

EmbedForTesting.prototype.addEventListener_ = function(message, callback) {
  if (!this.listeners[message]) {
    this.listeners[message] = [];
  }

  this.listeners[message].push(callback);
};

EmbedForTesting.prototype.load = function() {
  this.loaded = true;
  this.fireEvent('load', null);
};

EmbedForTesting.prototype.message = function(msg) {
  var event = {data: msg};
  this.fireEvent('message', event);
};

EmbedForTesting.prototype.error = function(error) {
  this.lastError = error;
  this.fireEvent('error', null);
};

EmbedForTesting.prototype.exit = function(exitStatus) {
  this.exitStatus = exitStatus;
  this.fireEvent('crash', null);
};

EmbedForTesting.prototype.crash = function() {
  this.exit(-1);
};

EmbedForTesting.prototype.addLoadListener = function(callback) {
  this.addEventListener_('load', callback);
};

EmbedForTesting.prototype.addMessageListener = function(callback) {
  this.addEventListener_('message', callback);
};

EmbedForTesting.prototype.addErrorListener = function(callback) {
  this.addEventListener_('error', callback);
};

EmbedForTesting.prototype.addCrashListener = function(callback) {
  this.addEventListener_('crash', callback);
};

EmbedForTesting.prototype.appendToBody = function() {
};

EmbedForTesting.prototype.setPostMessageCallback = function(callback) {
  this.postMessageCallback = callback;
}

EmbedForTesting.prototype.postMessage = function(msg) {
  assert(this.postMessageCallback);
  this.postMessageCallback(msg);
};

module.exports = EmbedForTesting;
