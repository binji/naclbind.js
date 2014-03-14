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


var z_stream = new nacl.StructType(16, 'z_stream');
z_stream.addField('next_in', nacl.uint8_p, 0);
z_stream.addField('avail_in', nacl.uint32, 4);
z_stream.addField('next_out', nacl.uint8_p, 8);
z_stream.addField('avail_out', nacl.uint32, 12);

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

var z_stream_p = new nacl.PointerType(z_stream);
var deflateType = new nacl.FunctionType(nacl.int32, z_stream_p, nacl.int32);
var deflateInit = new nacl.CFunction('deflateInit', deflateType);
var deflate = new nacl.CFunction('deflate', deflateType);

function compress(inputAb, level, bufferSize, callback) {
  var inputOffset = 0;
  var outputAb = null;
  var outputOffset = 0;

  var stream = z_stream.malloc();
  nacl.call(nacl.memset, stream, 0, z_stream.sizeof());

  var output = nacl.call(nacl.malloc, bufferSize);
  var result = nacl.call(deflateInit, stream, level);
  nacl.commit(result, step);

  function step(result) {
    // TODO check result. Need result to be converted from handle to primitive.

    var inputLeft = inputAb.byteLength - inputOffset;
    var inputSliceAb = sliceArrayBuffer(inputAb, inputOffset, bufferSize);
    var inputSlice = nacl.mapArrayBuffer(ab);

    z_stream.fields.next_in.set(stream, inputSlice);
    z_stream.fields.avail_in.set(stream, ab.byteLength);
    z_stream.fields.next_out.set(stream, output);
    z_stream.fields.avail_out.set(stream, bufferSize);

    var flush = inputLeft < bufferSize ? Z_PARTIAL_FLUSH : Z_FINISH;
    var result = nacl.call(deflate, stream, flush);
    var avail_in = z_stream.fields.avail_in.get(stream);
    var avail_out = z_stream.fields.avail_out.get(stream);

    nacl.commit(result, avail_in, avail_out, stepLoop);
  }

  function stepLoop(result, avail_in, avail_out) {
  }
}

var ab = new ArrayBuffer(10);
compress(ab, 6, 1024, function() { console.log('done!'); });
