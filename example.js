// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

"use strict";

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

//    z_const Bytef *next_in;     /* next input byte */
//    uInt     avail_in;  /* number of bytes available at next_in */
//    uLong    total_in;  /* total number of input bytes read so far */
//
//    Bytef    *next_out; /* next output byte should be put there */
//    uInt     avail_out; /* remaining free space at next_out */
//    uLong    total_out; /* total number of bytes output so far */
//
//    z_const char *msg;  /* last error message, NULL if no error */
//    struct internal_state FAR *state; /* not visible by applications */
//
//    alloc_func zalloc;  /* used to allocate the internal state */
//    free_func  zfree;   /* used to free the internal state */
//    voidpf     opaque;  /* private data object passed to zalloc and zfree */
//
//    int     data_type;  /* best guess about the data type: binary or text */
//    uLong   adler;      /* adler32 value of the uncompressed data */
//    uLong   reserved;   /* reserved for future use */

var z_stream = nacl.makeStructType(nacl.userTypeId + 0, 56, 'z_stream');
// TODO(binji): fields should be specified in the constructor.
z_stream.addField('next_in', nacl.uint8_p, 0);
z_stream.addField('avail_in', nacl.uint32, 4);
z_stream.addField('total_in', nacl.uint32, 8);
z_stream.addField('next_out', nacl.uint8_p, 12);
z_stream.addField('avail_out', nacl.uint32, 16);
z_stream.addField('total_out', nacl.uint32, 20);

var Z_NO_FLUSH = 0
var Z_PARTIAL_FLUSH = 1
var Z_SYNC_FLUSH = 2
var Z_FULL_FLUSH = 3
var Z_FINISH = 4
var Z_BLOCK = 5
var Z_TREES = 6

var Z_OK = 0
var Z_STREAM_END = 1
var Z_NEED_DICT = 2
var Z_ERRNO = -1
var Z_STREAM_ERROR = -2
var Z_DATA_ERROR = -3
var Z_MEM_ERROR = -4
var Z_BUF_ERROR = -5
var Z_VERSION_ERROR = -6

var z_stream_p = nacl.makePointerType(nacl.userTypeId + 1, z_stream);
var deflateType = nacl.makeFunctionType(nacl.userTypeId + 2, nacl.int32, z_stream_p, nacl.int32);
var deflateInit = nacl.makeFunction('deflateInit', deflateType);
var deflate = nacl.makeFunction('deflate', deflateType);

// nacl.logTypes();

function commitPromise() {
  var args = Array.prototype.slice.call(arguments);
  return new Promise(function(resolve) {
    args.push(function() {
      resolve(arguments);
    });
    nacl.commit.apply(null, args);
  });
}

function compress(inputAb, level, bufferSize) {
  var stream;
  var output;
  var inputOffset = 0;
  var outputAb = null;
  var outputOffset = 0;

  return Promise.resolve().then(function() {
    stream = z_stream.malloc().cast(z_stream_p);
    nacl.memset(stream, 0, z_stream.sizeof());

    output = nacl.malloc(bufferSize);
    var result = deflateInit(stream, level);
    return commitPromise(result);
  }).then(function(args) {
    var result = args[0];
    if (result !== Z_OK) {
      return Promise.reject([result]);
    }
  }).then(function() {
    function deflateMore() {
      var inputOffsetEnd = inputOffset + bufferSize;
      var inputSliceAb = sliceArrayBuffer(inputAb, inputOffset, inputOffsetEnd);
      var inputSlice = nacl.arrayBufferMap(inputSliceAb);
      z_stream.fields.next_in.set(stream, inputSlice.cast(nacl.uint8_p));
      z_stream.fields.avail_in.set(stream, inputSliceAb.byteLength);
      z_stream.fields.next_out.set(stream, output.cast(nacl.uint8_p));
      z_stream.fields.avail_out.set(stream, bufferSize);
      return deflateContinue();
    }

    function deflateContinue() {
      var inputLeft = inputAb.byteLength - inputOffset;
      var flush = inputLeft < bufferSize ? Z_FINISH : Z_NO_FLUSH;
      var preAvailIn = z_stream.fields.avail_in.get(stream);
      var result = deflate(stream, flush);
      var availIn = z_stream.fields.avail_in.get(stream);
      var availOut = z_stream.fields.avail_out.get(stream);
      var outUsed = nacl.sub(bufferSize, availOut);
      var compressedAb = nacl.arrayBufferCreate(outUsed);
      var outputAbPtr = nacl.arrayBufferMap(compressedAb);
      nacl.memcpy(outputAbPtr, output, outUsed);
      return commitPromise(result, preAvailIn, availIn, availOut, compressedAb);
    }

    function onDeflate(args) {
      var result = args[0];
      var preAvailIn = args[1];
      var availIn = args[2];
      var availOut = args[3];
      var compressedAb = args[4];

      if (result !== Z_OK && result !== Z_STREAM_END) {
        return Promise.reject([result]);
      }

      // Consume output.
      outputAb = copyToArrayBuffer(outputAb, outputOffset, compressedAb);
      outputOffset += compressedAb.byteLength;

      if (result === Z_STREAM_END) {
        return Promise.resolve([outputAb]);
      }

      if (availIn === 0) {
        // input underflow. Provide more input.
        inputOffset += preAvailIn;
        return deflateMore().then(onDeflate);
      } else if (availOut === 0) {
        // output overflow. Finish current input.
        return deflateContinue().then(onDeflate);
      } else {
        // Not possible...?
        console.log('onDeflate called with availOut = ' + availOut +
                    ' and availIn = ' + availIn + '?');
        return Promise.reject([null]);
      }
    }

    return deflateMore().then(onDeflate);
  }).then(function(args) {
    var outputAb = args[0];
    return outputAb;
  }).catch(function(args) {
    var result = args[0];
    nacl.free(stream);
    nacl.free(output);
    return commitPromise().then(function() {
      return Promise.reject(result);
    });
  });
}


function makeTestArrayBuffer(length, add, mul) {
  var newAb = new ArrayBuffer(length);
  var view = new Uint8Array(newAb);
  var value = 0;
  for (var i = 0; i < length; ++i) {
    value = ((value + add) * mul) | 0;
    view[i] = value & 255;
  }
  return newAb;
}


var ab = makeTestArrayBuffer(16384, 1337, 0xc0dedead);
compress(ab, 9, 2048).then(function(outputAb) {
  var before = ab.byteLength;
  var after = outputAb.byteLength;
  console.log('done! orig = ' + before +
              ' comp = ' + after +
              ' ratio = ' + ((after / before) * 100).toFixed(1) + '%');
}).catch(function(err) {
  console.log('done... error ' + err);
});
