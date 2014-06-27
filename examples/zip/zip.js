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
  var f = m.functions;

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
      that.zipFile = f.zipOpen(that.c, that.filename, 0);  // 0 = append
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
      var result = f.zipOpenNewFileInZip(c,
        that.zipFile, name, null, null, 0, null, 0, null, Z_DEFLATED,
        Z_DEFAULT_COMPRESSION);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result != ZIP_OK) {
        return promise.reject('zipOpenNewFileInZip('+name+') failed.');
      }

      return readBlobPromise(blob);
    }).then(function(blobAb) {
      var buf = f.arrayBufferMap(c, blobAb);
      var len = blob.size;
      var result = f.zipWriteInFileInZip(c, that.zipFile, buf, len);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result != ZIP_OK) {
        return promise.reject('zipWriteInFileInZip('+name+') failed.');
      }
    }).finally(function() {
      f.zipCloseFileInZip(c, that.zipFile);
      // TODO(binji): Can this be encoded in the command buffer?
      m.destroyHandles(c);
      return m.commitPromise();
    });
  };

  Zipper.prototype.close = function() {
    var that = this;
    this.p = this.p.then(function() {
      // TODO(binji): handle errors
      f.zipClose(that.c, that.zipFile, null);

      // Get the zip filename.

      // Get its size.
      var statbuf = m.mallocType(that.c, t.stat);
      f.stat(that.c, that.filename, statbuf);
      var size = m.getField(that.c, t.stat.fields.st_size, statbuf);

      // Create an ArrayBuffer of that size.
      var ab = f.arrayBufferCreate(that.c, size);
      var abPtr = f.arrayBufferMap(that.c, ab);

      // Open the zip file.
      var file = f.fopen(that.c, that.filename, "r");

      // Read the data into the ArrayBuffer.
      f.fread(that.c, abPtr, 1, size, file);
      f.fclose(that.c, file);

      return m.commitPromise(ab);
    }).then(function(zippedAb) {
      return promise.resolve(zippedAb);
    }).finally(function() {
      m.destroyHandles(that.c);
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
