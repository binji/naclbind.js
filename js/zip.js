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

define(['promise', 'nacl'], function(promise, nacl) {

  var m = nacl.makeModule(
      'zip-nacl', 'pnacl/Release/zip.nmf', 'application/x-pnacl');
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

  m.makeStructType(80, 'tm_zip_s', 24, {
    tm_sec: {type: t.uint32, offset: 0},
    tm_min: {type: t.uint32, offset: 4},
    tm_hour: {type: t.uint32, offset: 8},
    tm_mday: {type: t.uint32, offset: 12},
    tm_mon: {type: t.uint32, offset: 16},
    tm_year: {type: t.uint32, offset: 20},
  });

  m.makeStructType(81, 'zip_fileinfo', 36, {
    tmz_date: {type: t.tm_zip_s, offset: 0},
    dosDate: {type: t.uint32, offset: 24},
    internal_fa: {type: t.uint32, offset: 28},
    external_fa: {type: t.uint32, offset: 32},
  });
  m.makePointerType(82, t.zip_fileinfo);

  m.makeStructType(83, 'zipFile', 0, {});
  m.makePointerType(84, t.zipFile);
  var char$ = t.uint8$;

  m.makeFunction('zipOpen', m.makeFunctionType(85, t.zipFile$, char$, t.int32));
  m.makeFunction('zipOpenNewFileInZip',
                 m.makeFunctionType(86, t.int32, t.zipFile$, char$,
                                    t.zip_fileinfo$,
                                    t.void$, t.uint32,
                                    t.void$, t.uint32,
                                    char$, t.int32, t.int32));
  m.makeFunction('zipWriteInFileInZip',
                 m.makeFunctionType(87, t.int32, t.zipFile$, t.void$,
                                    t.uint32));
  m.makeFunction('zipCloseFileInZip',
                 m.makeFunctionType(88, t.int32, t.zipFile$));
  m.makeFunction('zipClose',
                 m.makeFunctionType(89, t.int32, t.zipFile$, char$));

  m.makeStructType(90, 'stat', 104, {
    st_dev: {type: t.uint64, offset: 0},
    st_ino: {type: t.uint64, offset: 8},
    st_mode: {type: t.uint32, offset: 16},
    st_nlink: {type: t.uint32, offset: 20},
    st_uid: {type: t.uint32, offset: 24},
    st_gid: {type: t.uint32, offset: 28},
    st_rdev: {type: t.uint32, offset: 32},
    // st_size: {type: t.uint64, offset: 40},
    // TODO(binji): add 64-bit int support so we can use the real type.
    st_size: {type: t.uint32, offset: 40},
    st_blksize: {type: t.uint32, offset: 48},
    st_blocks: {type: t.uint32, offset: 52},
    st_atime: {type: t.uint64, offset: 56},
    st_mtime: {type: t.uint64, offset: 72},
    st_ctime: {type: t.uint64, offset: 88},
  });
  m.makePointerType(91, t.stat);

  m.makeStructType(92, 'FILE', 0, {});
  m.makePointerType(93, t.FILE);
  m.makeFunction('fopen', m.makeFunctionType(94, t.FILE$, char$, char$));
  m.makeFunction('fread', m.makeFunctionType(95, t.int32, t.void$, t.uint32,
                                             t.uint32, t.FILE$));
  m.makeFunction('fclose', m.makeFunctionType(96, t.int32, t.FILE$));
  m.makeFunction('stat', m.makeFunctionType(97, t.int32, char$, t.stat$));

  var numFiles = 0;

  function Zipper() {
    this.filename = numFiles + '.zip';
    numFiles++;

    var that = this;
    this.c = m.makeContext();
    this.p = promise.resolve().then(function() {
      // TODO(binji): properly handle PP_Var strings. They are not necessarily
      // NULL-terminated (anywhere we call varToUtf8).
      var lenPtr = m.mallocType(that.c, t.uint32);
      var filenameCstr = f.varToUtf8(that.c, that.filename, lenPtr);
      that.zipFile = f.zipOpen(that.c, filenameCstr, 0);  // 0 = append
      f.free(that.c, lenPtr);
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
      var lenPtr = m.mallocType(c, t.uint32);
      var nameCstr = f.varToUtf8(c, name, lenPtr);
      var result = f.zipOpenNewFileInZip(c,
        that.zipFile, nameCstr, null, null, 0, null, 0, null, Z_DEFLATED,
        Z_DEFAULT_COMPRESSION);
      f.free(c, lenPtr);
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
      var lenPtr = m.mallocType(that.c, t.uint32);
      var filenameCstr = f.varToUtf8(that.c, that.filename, lenPtr);

      // Get its size.
      var statbuf = m.mallocType(that.c, t.stat);
      f.stat(that.c, filenameCstr, statbuf);
      var size = m.getField(that.c, t.stat.fields.st_size, statbuf);

      // Create an ArrayBuffer of that size.
      var ab = f.arrayBufferCreate(that.c, size);
      var abPtr = f.arrayBufferMap(that.c, ab);

      // Open the zip file.
      var modeCstr = f.varToUtf8(that.c, "r", lenPtr);
      var file = f.fopen(that.c, filenameCstr, modeCstr);
      f.free(that.c, lenPtr);

      // Read the data into the ArrayBuffer.
      f.fread(that.c, abPtr, 1, size, file);
      f.fclose(that.c, file);

      return m.commitPromise(ab);
    }).then(function(zippedAb) {
      return promise.resolve(zippedAb);
    }).finally(function() {
      m.destroyHandles(that.c);
      that.c = null;
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
