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

define(['promise', 'nacl', 'zip_glue'], function(promise, nacl, zip_glue) {
  var m = zip_glue;
  var t = m.types;

  var Z_DEFLATED = 8;
  var Z_DEFAULT_COMPRESSION = -1;

  var ZIP_OK = 0;
  var ZIP_EOF = 0;
  var ZIP_ERRNO = -1;
  var ZIP_PARAMERROR = -102;
  var ZIP_BADZIPFILE = -103;
  var ZIP_INTERNALERROR = -104;

  var numFiles = 0;

  function Zipper() {
    this.filename = numFiles + '.zip';
    numFiles++;

    var that = this;
    this.c = m.makeContext();
    this.p = promise.resolve().then(function() {
      that.zipFile = that.c.zipOpen(that.filename, 0);  // 0 = append
      return m.commitPromise();
    });
  }

  function readBlobPromise(blob) {
    return new promise.PromisePlus(function(resolve, reject) {
      var reader = new FileReader();
      reader.onloadend = function() {
        resolve(reader.result);
      };
      reader.onerror = function() {
        reject(reader.error);
      };
      reader.readAsArrayBuffer(blob);
    });
  }

  Zipper.prototype.addBlob = function(name, blob) {
    var that = this;
    var c = m.makeContext();
    this.p = this.p.then(function() {
      // TODO(binji): Stack allocation of handles...?
      // TODO(binji): If error occurs, debugging info...?
      var result = c.zipOpenNewFileInZip(
        that.zipFile, name, null, null, 0, null, 0, null, Z_DEFLATED,
        Z_DEFAULT_COMPRESSION);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result != ZIP_OK) {
        return promise.reject('zipOpenNewFileInZip('+name+') failed.');
      }

      return readBlobPromise(blob);
    }).then(function(blobAb) {
      var buf = c.arrayBufferMap(blobAb);
      var len = blob.size;
      var result = c.zipWriteInFileInZip(that.zipFile, buf, len);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result != ZIP_OK) {
        return promise.reject('zipWriteInFileInZip('+name+') failed.');
      }
    }).finally(function() {
      c.zipCloseFileInZip(that.zipFile);
      // TODO(binji): Can this be encoded in the command buffer?
      c.$destroyHandles();
      return m.commitPromise();
    });
  };

  Zipper.prototype.close = function() {
    var that = this;
    this.p = this.p.then(function() {
      // TODO(binji): handle errors
      that.c.zipClose(that.zipFile, null);

      // Get the zip filename.

      // Get its size.
      var statbuf = that.c.$mallocType(t.stat);
      that.c.stat(that.filename, statbuf);
      var size = that.c.$getField(t.stat.fields.st_size, statbuf);

      // Create an ArrayBuffer of that size.
      var ab = that.c.arrayBufferCreate(size);
      var abPtr = that.c.arrayBufferMap(ab);

      // Open the zip file.
      var file = that.c.fopen(that.filename, "r");

      // Read the data into the ArrayBuffer.
      that.c.fread(abPtr, 1, size, file);
      that.c.fclose(file);

      return m.commitPromise(ab);
    }).then(function(zippedAb) {
      return promise.resolve(zippedAb);
    }).finally(function() {
      that.c.$destroyHandles();
      that.c = null;
      // TODO(binji): Common pattern... simplify into a function?
      m.commitPromise();
    });
  };

  Zipper.prototype.getZipped = function(callback, errback) {
    this.p.then(callback, errback);
  };

  // Exports
  return {
    Zipper: Zipper,
  };
});
