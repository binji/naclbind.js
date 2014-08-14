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

define(['promise', 'zlib_glue'], function(promise, zlib_glue) {

  function sliceArrayBuffer(ab, begin, end) {
    var oldLength = ab.byteLength;

    if (begin === undefined) {
      begin = 0;
    } else if (begin < 0) {
      begin += oldLength;
    }

    begin = Math.max(0, Math.min(begin, oldLength));

    if (end === undefined) {
      end = oldLength;
    } else if (end < 0) {
      end += oldLength;
    }

    end = Math.max(0, Math.min(end, oldLength));

    var length = end - begin;
    if (length <= 0) {
      return new ArrayBuffer();
    }

    var newAb = new ArrayBuffer(length);
    (new Uint8Array(newAb)).set(new Uint8Array(ab, begin, length));
    return newAb;
  }

  function resizeArrayBuffer(ab, newByteLength) {
    var newAb = new ArrayBuffer(newByteLength);
    (new Uint8Array(newAb)).set(
        new Uint8Array(ab, 0, Math.min(newByteLength, ab.byteLength)));
    return newAb;
  }

  function copyToArrayBuffer(dst, dstOffset, src) {
    var srcLength = src.byteLength;
    if (dst === null) {
      dst = new ArrayBuffer(srcLength);
    }

    var minLength = dstOffset + srcLength;
    if (minLength > dst.byteLength) {
      dst = resizeArrayBuffer(dst, minLength * 2);
    }

    (new Uint8Array(dst)).set(src, dstOffset);
    return dst;
  }

  var Z_NO_FLUSH = 0;
  var Z_PARTIAL_FLUSH = 1;
  var Z_SYNC_FLUSH = 2;
  var Z_FULL_FLUSH = 3;
  var Z_FINISH = 4;
  var Z_BLOCK = 5;
  var Z_TREES = 6;

  var Z_OK = 0;
  var Z_STREAM_END = 1;
  var Z_NEED_DICT = 2;
  var Z_ERRNO = -1;
  var Z_STREAM_ERROR = -2;
  var Z_DATA_ERROR = -3;
  var Z_MEM_ERROR = -4;
  var Z_BUF_ERROR = -5;
  var Z_VERSION_ERROR = -6;

  var m = zlib_glue;
  var t = m.types;

  function zlibVersion() {
    var c = m.makeContext();
    return promise.resolve().then(function() {
      var version = c.zlibVersion();
      var versionLen = c.strlen(version);
      var result = c.varFromUtf8(version, versionLen);
      return m.commitPromise(result);
    }).then(function(result) {
      return promise.resolve(result);
    }).finally(function() {
      c.$destroyHandles();
      return m.commitPromise();
    });
  }

  function compressEasy(inputAb) {
    var c = m.makeContext();
    var dest = null;
    var destLenPtr = null;
    return promise.resolve().then(function() {
      var sourceLen = inputAb.byteLength;
      var destLenBound = c.compressBound(sourceLen);
      dest = c.$malloc(destLenBound).cast(t.uint8$);
      var source = c.arrayBufferMap(inputAb).cast(t.uint8$);
      destLenPtr = c.$mallocType(t.uint32);
      c.set(destLenPtr, destLenBound);
      var result = c.compress(dest, destLenPtr, source, sourceLen);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result !== Z_OK) {
        return promise.reject(result);
      }

      var destLen = c.get(destLenPtr);
      var destAb = c.arrayBufferCreate(destLen);
      var destAbPtr = c.arrayBufferMap(destAb);
      c.memcpy(destAbPtr, dest, destLen);
      return m.commitPromise(destAb);
    }).then(function(destAb) {
      return promise.resolve(destAb);
    }).finally(function() {
      c.$destroyHandles();
      return m.commitPromise();
    });
  }

  function compressHard(inputAb, level, bufferSize) {
    var stream = null;
    var output = null;
    var inputOffset = 0;
    var outputAb = null;
    var outputOffset = 0;

    var c = m.makeContext();

    return promise.resolve().then(function() {
      stream = c.$mallocType(t.z_stream);
      c.memset(stream, 0, t.z_stream.sizeof());

      output = c.$malloc(bufferSize);
      var result = c.deflateInit(stream, level);
      return m.commitPromise(result);
    }).then(function(result) {
      if (result !== Z_OK) {
        return promise.reject(result);
      }
      return promise.resolveMany(Z_OK, inputAb.byteLength, bufferSize, 0, null);
    }).while(function cond(result, availIn, availOut) {
      if (result !== Z_OK && result !== Z_STREAM_END) {
        return promise.reject(result);
      }

      return result !== Z_STREAM_END;
    }, function block(result, availIn, availOut, preAvailIn, compressedAb) {
      // Consume output, if any.
      if (compressedAb) {
        outputAb = copyToArrayBuffer(outputAb, outputOffset, compressedAb);
        outputOffset += compressedAb.byteLength;
      }

      var inputUnderflow = availIn === 0;
      var outputOverflow = availOut === 0;
      var initial = !inputUnderflow && !outputOverflow;

      if (inputUnderflow || initial) {
        // More room in the output buffer. Provide more input.
        inputOffset += preAvailIn;
        var inputOffsetEnd = inputOffset + bufferSize;
        var inputSliceAb = sliceArrayBuffer(inputAb, inputOffset, inputOffsetEnd);
        var inputSlice = c.arrayBufferMap(inputSliceAb);
        c.$setField(t.z_stream.fields.next_in, stream, inputSlice.cast(t.uint8$));
        c.$setField(t.z_stream.fields.avail_in, stream, inputSliceAb.byteLength);
        c.$setField(t.z_stream.fields.next_out, stream, output.cast(t.uint8$));
        c.$setField(t.z_stream.fields.avail_out, stream, bufferSize);
      }

      var inputLeft = inputAb.byteLength - inputOffset;
      var flush = inputLeft < bufferSize ? Z_FINISH : Z_NO_FLUSH;
      var preAvailIn = c.$getField(t.z_stream.fields.avail_in, stream);
      var result = c.deflate(stream, flush);
      var availIn = c.$getField(t.z_stream.fields.avail_in, stream);
      var availOut = c.$getField(t.z_stream.fields.avail_out, stream);
      var outUsed = c.sub(bufferSize, availOut);
      var compressedAb = c.arrayBufferCreate(outUsed);
      var outputAbPtr = c.arrayBufferMap(compressedAb);
      c.memcpy(outputAbPtr, output, outUsed);
      return m.commitPromise(result, availIn, availOut, preAvailIn, compressedAb);
    }).then(function(result, availIn, availOut, preAvailIn, compressedAb) {
      // Consume the final bit of output, if any.
      if (compressedAb) {
        outputAb = copyToArrayBuffer(outputAb, outputOffset, compressedAb);
        outputOffset += compressedAb.byteLength;
      }

      return promise.resolve(outputAb);
    }).finally(function() {
      c.$destroyHandles();
      return m.commitPromise();
    });
  }

  // Exports
  return {
    Z_NO_FLUSH: Z_NO_FLUSH,
    Z_PARTIAL_FLUSH: Z_PARTIAL_FLUSH,
    Z_SYNC_FLUSH: Z_SYNC_FLUSH,
    Z_FULL_FLUSH: Z_FULL_FLUSH,
    Z_FINISH: Z_FINISH,
    Z_BLOCK: Z_BLOCK,
    Z_TREES: Z_TREES,

    Z_OK: Z_OK,
    Z_STREAM_END: Z_STREAM_END,
    Z_NEED_DICT: Z_NEED_DICT,
    Z_ERRNO: Z_ERRNO,
    Z_STREAM_ERROR: Z_STREAM_ERROR,
    Z_DATA_ERROR: Z_DATA_ERROR,
    Z_MEM_ERROR: Z_MEM_ERROR,
    Z_BUF_ERROR: Z_BUF_ERROR,
    Z_VERSION_ERROR: Z_VERSION_ERROR,

    compressEasy: compressEasy,
    compressHard: compressHard,
    zlibVersion: zlibVersion,
  };

});
