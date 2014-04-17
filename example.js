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

// Types + Functions only needed for this example.

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

var z_stream = nacl.makeStructType(65, 56, 'z_stream');
var z_stream_p = nacl.makePointerType(66, z_stream);
var deflateType = nacl.makeFunctionType(67, nacl.int32, z_stream_p, nacl.int32);
var compressType = nacl.makeFunctionType(68, nacl.int32, nacl.uint8_p, nacl.uint32_p, nacl.uint8_p, nacl.uint32);
var compressBoundType = nacl.makeFunctionType(69, nacl.uint32, nacl.uint32);

// TODO(binji): fields should be specified in the constructor.
z_stream.addField('next_in', nacl.uint8_p, 0);
z_stream.addField('avail_in', nacl.uint32, 4);
z_stream.addField('total_in', nacl.uint32, 8);
z_stream.addField('next_out', nacl.uint8_p, 12);
z_stream.addField('avail_out', nacl.uint32, 16);
z_stream.addField('total_out', nacl.uint32, 20);

nacl.makeFunction('deflateInit', deflateType);
nacl.makeFunction('deflate', deflateType);
nacl.makeFunction('compress', compressType);
nacl.makeFunction('compressBound', compressBoundType);

// nacl.logTypes();

function compressEasy(inputAb) {
  var c = nacl.makeContext();
  var dest;
  var destLenPtr;
  return promise.resolve().then(function() {
    var sourceLen = inputAb.byteLength;
    var destLenBound = c.func.compressBound(sourceLen);
    dest = c.func.malloc(destLenBound).cast(nacl.uint8_p);
    var source = c.func.arrayBufferMap(inputAb).cast(nacl.uint8_p);
    destLenPtr = c.func.malloc(nacl.uint32.sizeof()).cast(nacl.uint32_p);
    c.func.set(destLenPtr, destLenBound);
    var result = c.func.compress(dest, destLenPtr, source, sourceLen);
    return c.commitPromise(result);
  }).then(function(result) {
    if (result !== Z_OK) {
      return promise.reject(result);
    }

    var destLen = c.func.get(destLenPtr);
    var destAb = c.func.arrayBufferCreate(destLen);
    var destAbPtr = c.func.arrayBufferMap(destAb);
    c.func.memcpy(destAbPtr, dest, destLen);
    return c.commitPromise(destAb);
  }).then(function(destAb) {
    return promise.resolve(destAb);
  }).finally(function() {
    c.func.free(dest);
    c.func.free(destLenPtr);
    return c.commitPromise();
  });
}

function compress(inputAb, level, bufferSize) {
  var stream;
  var output;
  var inputOffset = 0;
  var outputAb = null;
  var outputOffset = 0;

  var c = nacl.makeContext();

  return promise.resolve().then(function() {
    stream = z_stream.malloc(c).cast(z_stream_p);
    c.func.memset(stream, 0, z_stream.sizeof());

    output = c.func.malloc(bufferSize);
    var result = c.func.deflateInit(stream, level);
    return c.commitPromise(result);
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
      var inputSlice = c.func.arrayBufferMap(inputSliceAb);
      z_stream.fields.next_in.set(c, stream, inputSlice.cast(nacl.uint8_p));
      z_stream.fields.avail_in.set(c, stream, inputSliceAb.byteLength);
      z_stream.fields.next_out.set(c, stream, output.cast(nacl.uint8_p));
      z_stream.fields.avail_out.set(c, stream, bufferSize);
    }

    var inputLeft = inputAb.byteLength - inputOffset;
    var flush = inputLeft < bufferSize ? Z_FINISH : Z_NO_FLUSH;
    var preAvailIn = z_stream.fields.avail_in.get(c, stream);
    var result = c.func.deflate(stream, flush);
    var availIn = z_stream.fields.avail_in.get(c, stream);
    var availOut = z_stream.fields.avail_out.get(c, stream);
    var outUsed = c.func.sub(bufferSize, availOut);
    var compressedAb = c.func.arrayBufferCreate(outUsed);
    var outputAbPtr = c.func.arrayBufferMap(compressedAb);
    c.func.memcpy(outputAbPtr, output, outUsed);
    return c.commitPromise(result, availIn, availOut, preAvailIn, compressedAb);
  }).then(function(result, availIn, availOut, preAvailIn, compressedAb) {
    // Consume the final bit of output, if any.
    if (compressedAb) {
      outputAb = copyToArrayBuffer(outputAb, outputOffset, compressedAb);
      outputOffset += compressedAb.byteLength;
    }

    return promise.resolve(outputAb);
  }).finally(function() {
    c.func.free(stream);
    c.func.free(output);
    return c.commitPromise();
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
  console.log('compress done! orig = ' + before +
              ' comp = ' + after +
              ' ratio = ' + ((after / before) * 100).toFixed(1) + '%');
}).catch(function(err) {
  console.log('compress done...\n' + err.stack);
});

compressEasy(ab).then(function(outputAb) {
  var before = ab.byteLength;
  var after = outputAb.byteLength;
  console.log('compressEasy done! orig = ' + before +
              ' comp = ' + after +
              ' ratio = ' + ((after / before) * 100).toFixed(1) + '%');
}).catch(function(err) {
  console.log('compressEasy done...\n' + err.stack);
});


var id = function() { return promise.resolveMany.apply(null, arguments); }
var log = function() {
  console.log.apply(console, arguments);
  return promise.resolveMany.apply(null, arguments);
}
var ret = function(x) { return function() { return x; } };
var inc = function(x) { return x + 1; };
var lt = function(b) { return function(a) { return a < b; } };
var chain = function(f, g) {
  return function(x) { return promise.resolve(x).then(f).then(g); }
};
