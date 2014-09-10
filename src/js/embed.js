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

function Embed(nmf, mimeType) {
  this.nmf = nmf;
  this.mimeType = mimeType;
  this.element = document.createElement('embed');
  this.element.setAttribute('width', '0');
  this.element.setAttribute('height', '0');
  this.element.setAttribute('src', this.nmf);
  this.element.setAttribute('type', this.mimeType);
}

Embed.prototype.addEventListener_ = function(message, callback) {
  this.element.addEventListener(message, callback, false);
};

Embed.prototype.addLoadListener = function(callback) {
  this.addEventListener_('load', callback);
};

Embed.prototype.addMessageListener = function(callback) {
  this.addEventListener_('message', callback);
};

Embed.prototype.addErrorListener = function(callback) {
  this.addEventListener_('error', callback);
};

Embed.prototype.addCrashListener = function(callback) {
  this.addEventListener_('crash', callback);
};

Embed.prototype.appendToBody = function() {
  document.body.appendChild(this.element);
};

Embed.prototype.postMessage = function(msg) {
  this.element.postMessage(msg);
};

Object.defineProperty(Embed.prototype, 'lastError', {
  get: function() { return this.element.lastError; },
  enumerable: true
});

Object.defineProperty(Embed.prototype, 'exitStatus', {
  get: function() { return this.element.exitStatus; },
  enumerable: true
});

module.exports = Embed;
