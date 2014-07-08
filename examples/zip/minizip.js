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

  var APPEND_STATUS_CREATE = 0;
  var APPEND_STATUS_CREATEAFTER = 1;
  var APPEND_STATUS_ADDINZIP = 2;

  var numFiles = 0;


  function validateFilename(filename) {
    if (typeof(filename) !== 'string') {
      throw new Error('Filename should be string.');
    }
  }

  function validateFileinfo(fileinfo) {
    if (fileinfo.date && !(fileinfo.date instanceof Date)) {
      throw new Error('Unexpected value for date: ' + fileinfo.date);
    }

    if (fileinfo.attributes && typeof(fileinfo.attributes) !== 'number')) {
      throw new Error('Attributes should be a number.');
    }
  }

  function validateLevel(level) {
    if (!(typeof(level) === 'number')) {
      throw new Error('Level should be a number.');
    }

    if (!(level >= 0 && level <= 9)) {
      throw new Error('Level should be in the range [0, 9].');
    }
  }

  function validateRaw(raw) {
    if (!(typeof raw === 'boolean')) {
      throw new Error('Raw should be a boolean.');
    }
  }

  function validateComment(comment) {
    if (comment && !(typeof comment === 'string')) {
      throw new Error('Comment should be a string.');
    }
  }

  function validateData(data) {
    if (!(data instanceof ArrayBuffer)) {
      throw new Error('Data should be an ArrayBuffer.');
    }
  }


  /**
   */
  function Zip() {
    this.filename = numFiles + '.zip';
    numFiles++;

    var that = this;
    this.c = m.makeContext();
    this.p = promise.resolve().then(function() {
      var append = APPEND_STATUS_CREATE;
      that.zipFile = f.zipOpen(that.c, that.filename, append);
      return m.commitPromise();
    }).then(function(result) {
      if (result != ZIP_OK) {
        return promise.reject('Unable to open zipfile: ' + that.filename);
      }
    });
    this.opened = false;
  }

  /**
   */
  Zip.prototype.openFile_ = function(filename, opts) {
    validateFilename(filename);
    if (opts) {
      if (opts.fileinfo)
        validateFileinfo(opts.fileinfo);
      if (opts.level)
        validateLevel(opts.level);
      if (opts.raw)
        validateRaw(opts.raw);
      if (opts.comment)
        validateComment(opts.comments);
    }

    if (this.opened)
      throw new Error('Must close before opening a new file for writing.');

    this.opened = true;

    var that = this;
    this.p = this.p.then(function() {
      var comment = null;
      var level = Z_DEFAULT_COMPRESSION;
      var raw = 0;
      var c_fileinfo = null;

      if (opts) {
        if (opts.fileinfo) {
          var date = opts.fileinfo.date;
          if (!date)
            date = new Date();

          c_fileinfo = m.mallocType(that.c, t.zip_fileinfo);
          var tm_fields = t.zip_fileinfo.fields.tmz_date.fields;
          m.setField(that.c, tm_fields.tm_sec, c_fileinfo, date.getSeconds());
          m.setField(that.c, tm_fields.tm_min, c_fileinfo, date.getMinutes());
          m.setField(that.c, tm_fields.tm_hour, c_fileinfo, date.getHours());
          m.setField(that.c, tm_fields.tm_mday, c_fileinfo, date.getDate());
          m.setField(that.c, tm_fields.tm_mon, c_fileinfo, date.getMonth());
          m.setField(that.c, tm_fields.tm_year, c_fileinfo,
                     date.getFullYear() - 1980);
          m.setField(that.c, t.zip_fileinfo.fields.dosDate, 0);
          m.setField(that.c, t.zip_fileinfo.fields.internal_fa, 0);

          var attributes = opts.fileinfo.attributes || 0;
          m.setField(that.c, t.zip_fileinfo.fields.external_fa, attributes);
        }

        if (opts.comment) comment = opts.comment;
        if (opts.level) level = opts.level;
        if (opts.raw) raw = opts.raw ? 1 : 0;
      }

      var result = f.zipOpenNewFileInZip2(that.c,
                                          that.zipFile,
                                          filename,
                                          c_fileinfo,
                                          null, 0,  // extrafield_local
                                          null, 0,  // extrafield_global
                                          comment,
                                          Z_DEFLATED,  // method
                                          level,
                                          raw);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result != ZIP_OK) {
        promise.reject('zipOpenNewFileInZip2 failed. result = ' + result);
      }
    });
  };

  Zip.prototype.writeFile_ = function(ab) {
    validateData(data);
    var that = this;
    this.p = this.p.then(function() {
      var c_buf = f.arrayBufferMap(that.c, ab);
      var c_len = ab.byteLength;
      var result = f.zipWriteInFileInZip(that.c, that.zipFile, c_buf, c_len);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result != ZIP_OK) {
        promise.reject('zipWriteInFileInZip failed. result = ' + result);
      }
    });
  };

  Zip.prototype.closeFile_ = function() {
    var that = this;
    this.p = this.p.then(function() {
      f.zipCloseFileInZip(that.c, that.zipFile);
      return m.commitPromise();
    });
  };
});
