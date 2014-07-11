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

require(['promise', 'minizip'], function(promise, minizip) {

  var zip;
  var zipPromise;
  var state;
  var size = new Size('#size');
  var compressed = new Size('#compressed');
  var numFiles;

  function smoothstep(x) {
    return x*x*(3 - 2 * x);
  }

  function prettyByteSize(size) {
    var result = '';
    var count = -1;
    size = size.toString();
    for (var i = size.length - 1; i >= 0; i--) {
      if (++count === 3) {
        result = ',' + result;
        count = 0;
      }
      result = size[i] + result;
    }
    return result + ' bytes';
  }

  function Size(selector) {
    this.size = 0;
    this.startSize = 0;
    this.endSize = 0;
    this.startTime = null;
    this.endTime = null;
    this.id = -1;
    this.el = $(selector);
  }

  Size.prototype.addSize = function(value) {
    this.setSize(this.endSize + value);
  };

  Size.prototype.setSize = function(value) {
    if (this.id !== -1) {
      window.clearInterval(this.id);
    }

    this.startSize = this.size;
    this.endSize = value;
    this.id = window.setInterval(this.update.bind(this), 50);
    this.startTime = (new Date()).getTime();
    this.endTime = this.startTime + 1000;  // 1 sec.
    this.setChanging(true);
  };

  Size.prototype.setSizeImmediate = function(value) {
    if (this.id !== -1) {
      window.clearInterval(this.id);
    }

    this.size = this.startSize = this.endSize = value;
    this.updateLabel();
    this.setChanging(false);
  };

  Size.prototype.update = function() {
    var now = (new Date()).getTime();
    if (now > this.endTime) {
      window.clearInterval(this.id);
      this.id = -1;
      this.size = this.endSize;
      this.updateLabel();
      this.setChanging(false);
      return;
    }

    var percentElapsed = (now - this.startTime) / (this.endTime - this.startTime);
    this.size = (this.endSize - this.startSize) * smoothstep(percentElapsed) + this.startSize;
    this.updateLabel();
  };

  Size.prototype.setChanging = function(changing) {
    if (changing) {
      this.el.classList.add('changing');
    } else {
      this.el.classList.remove('changing');
    }
  }

  Size.prototype.updateLabel = function() {
    var text = '';
    if (this.size !== 0) {
      text = prettyByteSize(this.size|0);
    }
    this.el.textContent = text;
  };

  reset();
  size.updateLabel();

  function $(selector) {
    return document.querySelector(selector);
  }

  function reset() {
    zip = new minizip.Zip();
    zipPromise = zip.openPromise();
    state = 'compress';
    size.setSizeImmediate(0);
    compressed.setSizeImmediate(0);
    $('#size').classList.remove('left');
    $('#compressed').classList.add('gone');
    $('#percent').classList.add('gone');
    $('#uncompressedLabel').classList.add('gone');
    $('#compressedLabel').classList.add('gone');
    $('#percentLabel').classList.add('gone');
    numFiles = 0;
    updateLabel();
  }

  $('#dropTarget').addEventListener('click', onClickDropTarget, false);
  $('#dropTarget').addEventListener('dragleave', onDragLeave, false);
  $('#dropTarget').addEventListener('dragover', onDragOver, false);
  $('#dropTarget').addEventListener('drop', onDrop, false);

  function makeDownloadLink(ab) {
    var blob = new Blob([ab]);
    var url = webkitURL.createObjectURL(blob);
    var el = $('#downloadLink');
    el.setAttribute('href', url);
    el.setAttribute('download', 'download.zip');
  }

  function transitionPromise(el) {
    return new promise.PromisePlus(function(resolve) {
      el.addEventListener('webkitTransitionEnd', function listener(event) {
        el.removeEventListener('webkitTransitionEnd', listener);
        resolve();
      }, false);
    });
  }

  function waitPromise(timeMs) {
    return new promise.PromisePlus(function(resolve) {
      window.setTimeout(function() { resolve(); }, timeMs);
    });
  }

  function onClickDropTarget(e) {
    if (numFiles <= 0) {
      return;
    }

    if (state === 'compress') {
      zipPromise = zipPromise.then(function() {
        return zip.closePromise(null);
      }).then(function() {
        return zip.getDataPromise();
      }).then(function(zippedAb) {
        var compressedSize = zippedAb.byteLength;
        makeDownloadLink(zippedAb);

        var sizeEl = $('#size');
        var compressedEl = $('#compressed');
        var percentEl = $('#percent');

        // Move the uncompressed size to the left.
        sizeEl.classList.add('left');
        transitionPromise(sizeEl).then(function() {
          // Then show the compressed size.
          compressed.setSizeImmediate(size.endSize);
          compressedEl.classList.remove('gone');
          return transitionPromise(compressedEl);
        }).then(function() {
          // Wait a second for the size to countdown.
          compressed.setSize(compressedSize);
          return waitPromise(1000);
        }).then(function() {
          // Display the percentage.
          percentEl.textContent = (compressedSize * 100 / size.endSize).toFixed(2) + '%';
          percentEl.classList.remove('gone');
          return transitionPromise(percentEl);
        }).then(function() {
          // Finally display the labels.
          $('#uncompressedLabel').classList.remove('gone');
          $('#compressedLabel').classList.remove('gone');
          $('#percentLabel').classList.remove('gone');
        });
      });

      state = 'download';
      updateLabel();
    } else if (state === 'download') {
      $('#downloadLink').dispatchEvent(new MouseEvent('click'));
      reset();
    }
  }

  function onDragOver(e) {
    e.dataTransfer.dropEffect = 'copy';
    $('#dropTarget').classList.add('dragover');
    e.stopPropagation();
    e.preventDefault();
  }

  function onDragLeave(e) {
    $('#dropTarget').classList.remove('dragover');
    e.stopPropagation();
    e.preventDefault();
  }

  function onDrop(e) {
    Array.prototype.forEach.call(e.dataTransfer.files, function(file) {
      loadFile(file);
    });
    e.stopPropagation();
    e.preventDefault();
    $('#dropTarget').classList.remove('dragover');
  }

  function onFileChange(e) {
    Array.prototype.forEach.call($('input').files, function(file) {
      loadFile(file);
    });
  }

  function loadFile(file) {
    zipPromise = zipPromise.then(function() {
      return zip.writePromise(file.name, {fileinfo: {date: new Date()}}, file);
    }).then(function() {
      size.addSize(file.size);
      numFiles++;
      updateLabel();
    }).catch(function(err) {
      // TODO(binji): display error to user?
      console.error(err);
      throw err;
    });
  }

  function updateLabel() {
    var label = $('#dropLabel');
    if (numFiles > 0) {
      if (state === 'compress') {
        label.textContent = 'Compress!';
      } else {
        label.textContent = 'Download!';
      }
    } else {
      label.textContent = 'Drop!';
    }
  }

});
