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

define(['nacl', 'zip_glue'], function(nacl, zip_glue) {
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

  var UNZ_OK = 0;
  var UNZ_END_OF_LIST_OF_FILE = -100;

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

    if (fileinfo.attributes && typeof(fileinfo.attributes) !== 'number') {
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


  function Zip() {
    this.filename = null;
    this.c = m.makeContext();
    this.openedFile = false;
    this.h_zipFile = null;
  }

  Zip.prototype.open = function(callback, errback) {
    var that = this;

    // Generate a unique name in the NaCl modules memory filesystem to store
    // the generated zip file.
    this.filename = numFiles + '.zip';
    numFiles++;

    try {
      var h_zipFile = f.zipOpen(this.c, this.filename, APPEND_STATUS_CREATE);
      return m.commit(h_zipFile, function(h_zipFile) {
        that.h_zipFile = h_zipFile;
        callback();
      });
    } catch(e) {
      errback(e);
    }
  };

  Zip.prototype.write = function(filename, opts, data, callback, errback) {
    var that = this;

    this.openFile(filename, opts, function() {
      that.writeToFile(data, function() {
        that.closeFile(callback, errback);
      }, errback);
    }, errback);
  };

  Zip.prototype.openFile = function(filename, opts, callback, errback) {
    try {
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

      if (this.openedFile)
        throw new Error('Must close before opening a new file for writing.');

      this.openedFile = true;

      var comment = null;
      var level = Z_DEFAULT_COMPRESSION;
      var raw = 0;
      var h_fileinfo = null;

      if (opts) {
        if (opts.fileinfo) {
          var date = opts.fileinfo.date;
          if (!date)
            date = new Date();

          h_fileinfo = m.mallocType(this.c, t.zip_fileinfo);
          var tm_fields = t.zip_fileinfo.fields.tmz_date.fields;
          m.setField(this.c, tm_fields.tm_sec, h_fileinfo, date.getSeconds());
          m.setField(this.c, tm_fields.tm_min, h_fileinfo, date.getMinutes());
          m.setField(this.c, tm_fields.tm_hour, h_fileinfo, date.getHours());
          m.setField(this.c, tm_fields.tm_mday, h_fileinfo, date.getDate());
          m.setField(this.c, tm_fields.tm_mon, h_fileinfo, date.getMonth());
          m.setField(this.c, tm_fields.tm_year, h_fileinfo,
                     date.getFullYear() - 1980);
          m.setField(this.c, t.zip_fileinfo.fields.dosDate, h_fileinfo, 0);
          m.setField(this.c, t.zip_fileinfo.fields.internal_fa, h_fileinfo, 0);

          var attributes = opts.fileinfo.attributes || 0;
          m.setField(this.c, t.zip_fileinfo.fields.external_fa, h_fileinfo,
                     attributes);
        }

        if (opts.comment) comment = opts.comment;
        if (opts.level) level = opts.level;
        if (opts.raw) raw = opts.raw ? 1 : 0;
      }

      var h_result = f.zipOpenNewFileInZip2(this.c,
                                            this.h_zipFile,
                                            filename,
                                            h_fileinfo,
                                            null, 0,  // extrafield_local
                                            null, 0,  // extrafield_global
                                            comment,
                                            Z_DEFLATED,  // method
                                            level,
                                            raw);

      m.commit(h_result, function(result) {
        if (result != ZIP_OK) {
          errback(new Error('zipOpenNewFileInZip2 failed. result = ' + result));
          return;
        }

        callback();
      });
    } catch (e) {
      errback(e);
    }
  };

  Zip.prototype.writeToFile = function(data, callback, errback) {
    try {
      if (data instanceof ArrayBuffer) {
        this.writeArrayBufferToFile(data, callback, errback);
      } else if (data instanceof Blob) {
        this.writeBlobToFile(data, callback, errback);
      } else {
        throw new Error('Unexpected data type: ' + data);
      }
    } catch(e) {
      errback(e);
    }
  };

  Zip.prototype.writeBlobToFile = function(blob, callback, errback) {
    var that = this;

    try {
      var reader = new FileReader();
      reader.onload = function() {
        that.writeArrayBufferToFile(this.result, callback, errback);
      };
      reader.onerror = function() {
        errback(reader.error);
      };
      reader.readAsArrayBuffer(blob);
    } catch (e) {
      errback(e);
    }
  };

  Zip.prototype.writeArrayBufferToFile = function(ab, callback, errback) {
    try {
      var h_buf = f.arrayBufferMap(this.c, ab);
      var h_len = ab.byteLength;
      var h_result = f.zipWriteInFileInZip(this.c, this.h_zipFile,
                                           h_buf, h_len);
      m.commit(h_result, function(result) {
        if (result != ZIP_OK) {
          errback(new Error('zipWriteInFileInZip failed. result = ' + result));
          return;
        }

        callback();
      });
    } catch(e) {
      errback(e);
    }
  };

  Zip.prototype.closeFile = function(callback, errback) {
    try {
      this.openedFile = false;

      var h_result = f.zipCloseFileInZip(this.c, this.h_zipFile);
      m.commit(h_result, function(result) {
        if (result != ZIP_OK) {
          errback(new Error('zipCloseFileInZip failed. result = ' + result));
          return;
        }

        callback();
      });
    } catch(e) {
      errback(e);
    }
  };

  Zip.prototype.close = function(globalComment, callback, errback) {
    try {
      var h_result = f.zipClose(this.c, this.h_zipFile, globalComment);
      m.commit(h_result, function(result) {
        if (result != ZIP_OK) {
          errback(new Error('zipClose failed. result = ' + result));
          return;
        }

        callback();
      });
    } catch(e) {
      errback(e);
    }
  };

  Zip.prototype.getData = function(callback, errback) {
    var that = this;
    try {
      var h_statbuf = m.mallocType(this.c, t.stat);
      var h_result = f.stat(this.c, this.filename, h_statbuf);
      m.commit(h_result, function(result) {
        if (result != 0) {
          errback(new Error('stat(' + that.filename + ') failed.'));
          return;
        }

        var h_size = m.getField(that.c, t.stat.fields.st_size, h_statbuf);
        f.free(that.c, h_statbuf);
        var h_ab = f.arrayBufferCreate(that.c, h_size);
        var h_abPtr = f.arrayBufferMap(that.c, h_ab);
        var h_file = f.fopen(that.c, that.filename, "r");
        f.fread(that.c, h_abPtr, 1, h_size, h_file);
        f.arrayBufferUnmap(that.c, h_ab);
        f.fclose(that.c, h_file);

        m.commit(h_ab, function(ab) {
          m.destroyHandles(that.c);
          that.c = null;
          m.commit(function() {
            callback(ab);
          });
        });
      });
    } catch(e) {
      errback(e);
    }
  };

  function Unzip() {
    this.filename = null;
    this.c = m.makeContext();
    this.h_zipFile = null;
  }

  Unzip.prototype.open = function(zipFile, callback, errback) {
    try {
      if (zipFile instanceof ArrayBuffer) {
        this.openArrayBuffer(zipFile, callback, errback);
      } else if (data instanceof Blob) {
        this.openBlob(zipFile, callback, errback);
      } else {
        throw new Error('Unexpected zipFile type: ' + zipFile);
      }
    } catch(e) {
      errback(e);
    }
  };

  Unzip.prototype.openArrayBuffer = function(ab, callback, errback) {
    try {
      var h_abPtr = f.arrayBufferMap(this.c, ab);
      var h_file = f.fopen(this.c, this.filename, "w");
      f.fwrite(this.c, h_abPtr, 1, ab.byteLength, h_file);
      f.fclose(this.c, h_file);

      var h_zipFile = f.unzOpen(this.c, this.filename);
      m.commit(h_zipFile, function(h_zipFile) {
        if (result != UNZ_OK) {
          errback(new Error('unzIpen failed. result = ' + result));
          return;
        }

        callback();
      });
    } catch(e) {
      errback(e);
    }
  };

  Unzip.prototype.openBlob = function(blob, callback, errback) {
    var that = this;
    try {
      var reader = new FileReader();
      reader.onload = function() {
        that.openArrayBuffer(this.result, callback, errback);
      };
      reader.onerror = function() {
        errback(reader.error);
      };
      reader.readAsArrayBuffer(blob);
    } catch(e) {
      errback(e);
    }
  };

  Unzip.prototype.goToFirstFile = function(callback, errback) {
    try {
      var h_result = f.unzGoToFirstFile(this.c, this.h_zipFile);
      m.commit(h_result, function(result) {
        if (result != UNZ_OK) {
          errback(new Error('unzGoToFirstFile failed. result = ' + result));
          return;
        }

        callback();
      });
    } catch(e) {
      errback(e);
    }
  };

  Unzip.prototype.goToNextFile = function(callback, errback) {
    try {
      var h_result = f.unzGoToNextFile(this.c, this.h_zipFile);
      m.commit(h_result, function(result) {
        if (result == UNZ_END_OF_LIST_OF_FILE) {
          callback(false);
        } else if (result != UNZ_OK) {
          errback(new Error('unzGoToNextFile failed. result = ' + result));
        } else {
          callback(true);
        }
      });
    } catch(e) {
      errback(e);
    }
  };

  Unzip.prototype.locateFile = function(filename, caseSensitive,
                                        callback, errback) {
    try {
      var h_result = f.unzLocateFile(this.c, this.h_zipFile, filename,
                                     caseSensitive);
      m.commit(h_result, function(result) {
        if (result == UNZ_END_OF_LIST_OF_FILE) {
          callback(false);
        } else if (result != UNZ_OK) {
          errback(new Error('unzLocateFile failed. result = ' + result));
        } else {
          callback(true);
        }
      });
    } catch(e) {
      errback(e);
    }
  };

  Unzip.prototype.openCurrentFile = function(callback, errback) {
    try {
      var h_result = f.unzOpenCurrentFile(this.c, this.h_zipFile);
      m.commit(h_result, function(result) {
        if (result != UNZ_OK) {
          errback(new Error('unzOpenCurrentFile failed. result = ' + result));
          return;
        }

        callback();
      });
    } catch(e) {
      errback(e);
    }
  };

  Unzip.prototype.closeCurrentFile = function(callback, errback) {
    try {
      var h_result = f.unzCloseCurrentFile(this.c, this.h_zipFile);
      m.commit(h_result, function(result) {
        if (result != UNZ_OK) {
          errback(new Error('unzCloseCurrentFile failed. result = ' + result));
          return;
        }

        callback();
      });
    } catch(e) {
      errback(e);
    }
  };

  Unzip.prototype.unzReadCurrentFile = function(length, callback, errback) {
    try {
      var h_buffer = f.malloc(this.c, length);
      var h_result = f.unzReadCurrentFile(this.c, this.h_zipFile, h_buffer,
                                          length);
      var h_ab = f.arrayBufferCreate(this.c, length);
      var h_abPtr = f.arrayBufferMap(this.c, h_ab);
      f.memcpy(this.c, h_abPtr, h_buffer, length);
      f.arrayBufferUnmap(this.c, h_ab);
      f.free(this.c, h_buffer);

      m.commit(h_result, h_ab, function(result, ab) {
        if (result < 0) {
          errback(new Error('unzReadCurrentFile failed. result = ' + result));
          return;
        }

        callback(result);
      });
    } catch(e) {
      errback(e);
    }
  };


  // Promise-based API.
  function makePromiseFunction(func) {
    return function() {
      var that = this;
      var args = Array.prototype.slice.call(arguments);
      return new Promise(function(resolve, reject) {
        args.push(resolve);
        args.push(reject);
        func.apply(that, args);
      });
    };
  };

  Zip.prototype.openPromise = makePromiseFunction(Zip.prototype.open);
  Zip.prototype.writePromise = makePromiseFunction(Zip.prototype.write);
  Zip.prototype.openFilePromise = makePromiseFunction(Zip.prototype.openFile);
  Zip.prototype.writeToFilePromise =
      makePromiseFunction(Zip.prototype.writeToFile);
  Zip.prototype.writeBlobToFilePromise =
      makePromiseFunction(Zip.prototype.writeBlobToFile);
  Zip.prototype.writeArrayBufferToFilePromise =
      makePromiseFunction(Zip.prototype.writeArrayBufferToFile);
  Zip.prototype.closeFilePromise = makePromiseFunction(Zip.prototype.closeFile);
  Zip.prototype.closePromise = makePromiseFunction(Zip.prototype.close);
  Zip.prototype.getDataPromise = makePromiseFunction(Zip.prototype.getData);

  // Exports
  return {
    Zip: Zip
  };
});
