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

function NaClEmbed(nmf, mimeType) {
  if (!(this instanceof NaClEmbed)) { return new NaClEmbed(nmf, mimeType); }
  this.nmf = nmf;
  this.mimeType = mimeType;
  this.element = document.createElement('embed');
  this.element.setAttribute('width', '0');
  this.element.setAttribute('height', '0');
  this.element.setAttribute('src', this.nmf);
  this.element.setAttribute('type', this.mimeType);
}

NaClEmbed.prototype.addEventListener_ = function(message, callback) {
  this.element.addEventListener(message, callback, false);
};

NaClEmbed.prototype.addLoadListener = function(callback) {
  this.addEventListener_('load', callback);
};

NaClEmbed.prototype.addMessageListener = function(callback) {
  this.addEventListener_('message', callback);
};

NaClEmbed.prototype.addErrorListener = function(callback) {
  this.addEventListener_('error', callback);
};

NaClEmbed.prototype.addCrashListener = function(callback) {
  this.addEventListener_('crash', callback);
};

NaClEmbed.prototype.appendToBody = function() {
  document.body.appendChild(this.element);
};

NaClEmbed.prototype.postMessage = function(msg) {
  this.element.postMessage(msg);
};

Object.defineProperty(NaClEmbed.prototype, 'lastError', {
  get: function() { return this.element.lastError; },
  enumerable: true
});

Object.defineProperty(NaClEmbed.prototype, 'exitStatus', {
  get: function() { return this.element.exitStatus; },
  enumerable: true
});

module.exports = NaClEmbed;
