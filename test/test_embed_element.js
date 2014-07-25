var assert = require('assert');

function TestEmbedElement(nmf, mimeType) {
  this.loaded = false;
  this.nmf = nmf;
  this.mimeType = mimeType;
  this.listeners = {};
  this.postMessageCallback = null;
  this.lastError = undefined;
  this.exitStatus = undefined;
}

TestEmbedElement.prototype.fireEvent = function(message, e) {
  var callbacks = this.listeners[message];
  if (!callbacks) {
    return;
  }

  for (var i = 0; i < callbacks.length; ++i) {
    callbacks[i](e);
  }
};

TestEmbedElement.prototype.addEventListener_ = function(message, callback) {
  if (!this.listeners[message]) {
    this.listeners[message] = [];
  }

  this.listeners[message].push(callback);
};

TestEmbedElement.prototype.load = function() {
  this.loaded = true;
  this.fireEvent('load', null);
};

TestEmbedElement.prototype.message = function(msg) {
  this.fireEvent('message', msg);
};

TestEmbedElement.prototype.error = function(error) {
  this.lastError = error;
  this.fireEvent('error', null);
};

TestEmbedElement.prototype.exit = function(exitStatus) {
  this.exitStatus = exitStatus;
  this.fireEvent('crash', null);
};

TestEmbedElement.prototype.crash = function() {
  this.exit(-1);
};

TestEmbedElement.prototype.addLoadListener = function(callback) {
  this.addEventListener_('load', callback);
};

TestEmbedElement.prototype.addMessageListener = function(callback) {
  this.addEventListener_('message', callback);
};

TestEmbedElement.prototype.addErrorListener = function(callback) {
  this.addEventListener_('error', callback);
};

TestEmbedElement.prototype.addCrashListener = function(callback) {
  this.addEventListener_('crash', callback);
};

TestEmbedElement.prototype.appendToBody = function() {
};

TestEmbedElement.prototype.setPostMessageCallback = function(callback) {
  this.postMessageCallback = callback;
}

TestEmbedElement.prototype.postMessage = function(msg) {
  assert(this.postMessageCallback);
  this.postMessageCallback(msg);
};

module.exports = TestEmbedElement;
