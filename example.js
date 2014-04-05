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

var void_ = nacl.makeVoidType(1);
var int8 = nacl.makePrimitiveType(2, 'int8', 1, true, true);
var uint8 = nacl.makePrimitiveType(3, 'uint8', 1, false, true);
var int16 = nacl.makePrimitiveType(4, 'int16', 2, true, true);
var uint16 = nacl.makePrimitiveType(5, 'uint16', 2, false, true);
var int32 = nacl.makePrimitiveType(6, 'int32', 4, true, true);
var uint32 = nacl.makePrimitiveType(7, 'uint32', 4, false, true);
var int64 = nacl.makePrimitiveType(8, 'int64', 8, true, true);
var uint64 = nacl.makePrimitiveType(9, 'uint64', 8, false, true);
var float32 = nacl.makePrimitiveType(10, 'float32', 4, false, false);
var float64 = nacl.makePrimitiveType(11, 'float64', 8, false, false);
var size_t = nacl.makeAliasType(12, 'size_t', uint32);

var void_p = nacl.makePointerType(13, void_);
var uint8_p = nacl.makePointerType(14, uint8);
var uint8_pp = nacl.makePointerType(15, uint8_p);
var uint32_p = nacl.makePointerType(16, uint32);

var var_ = nacl.makePepperType(17, 'Var', undefined);
var arrayBuffer = nacl.makePepperType(18, 'ArrayBuffer', ArrayBuffer);
var array = nacl.makePepperType(19, 'Array', Array);
var dictionary = nacl.makePepperType(20, 'Dictionary', Object);
var addRefReleaseType = nacl.makeFunctionType(21, void_, var_);
var freeType = nacl.makeFunctionType(22, void_, void_p);
var mallocType = nacl.makeFunctionType(23, void_p, size_t);
var memsetType = nacl.makeFunctionType(24, void_, void_p, int32, size_t);
var memcpyType = nacl.makeFunctionType(25, void_, void_p, void_p, size_t);
var addVoidpInt32Type = nacl.makeFunctionType(26, void_p, void_p, int32);
var setUint8pType = nacl.makeFunctionType(27, void_, uint8_pp, uint8_p);
var setUint32Type = nacl.makeFunctionType(28, void_, uint32_p, uint32);
var getUint8pType = nacl.makeFunctionType(29, uint8_p, uint8_pp);
var getUint32Type = nacl.makeFunctionType(30, uint32, uint32_p);
var subInt32Type = nacl.makeFunctionType(31, int32, int32, int32);
var subUint32Type = nacl.makeFunctionType(32, uint32, uint32, uint32);

var arrayBufferCreateType = nacl.makeFunctionType(33, arrayBuffer, uint32);
var arrayBufferMapType = nacl.makeFunctionType(34, void_p, arrayBuffer);
var arrayBufferUnmapType = nacl.makeFunctionType(35, void_, arrayBuffer);

// Built-in functions.
var add = nacl.makeFunction('add', addVoidpInt32Type);
var addRef = nacl.makeFunction('addRef', addRefReleaseType);
var arrayBufferCreate = nacl.makeFunction('arrayBufferCreate', arrayBufferCreateType);
var arrayBufferMap = nacl.makeFunction('arrayBufferMap', arrayBufferMapType);
var free = nacl.makeFunction('free', freeType);
var get = nacl.makeFunction('get', [getUint8pType, getUint32Type]);
var malloc = nacl.makeFunction('malloc', mallocType);
var memcpy = nacl.makeFunction('memcpy', memcpyType);
var memset = nacl.makeFunction('memset', memsetType);
var release = nacl.makeFunction('release', addRefReleaseType);
var set = nacl.makeFunction('set', [setUint8pType, setUint32Type]);
var sub = nacl.makeFunction('sub', [subInt32Type, subUint32Type]);
var z_stream = nacl.makeStructType(36, 56, 'z_stream');
// TODO(binji): fields should be specified in the constructor.
z_stream.addField('next_in', uint8_p, 0);
z_stream.addField('avail_in', uint32, 4);
z_stream.addField('total_in', uint32, 8);
z_stream.addField('next_out', uint8_p, 12);
z_stream.addField('avail_out', uint32, 16);
z_stream.addField('total_out', uint32, 20);

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

var z_stream_p = nacl.makePointerType(37, z_stream);
var deflateType = nacl.makeFunctionType(38, int32, z_stream_p, int32);
var deflateInit = nacl.makeFunction('deflateInit', deflateType);
var deflate = nacl.makeFunction('deflate', deflateType);

// nacl.logTypes();

function compress(inputAb, level, bufferSize) {
  var stream;
  var output;
  var inputOffset = 0;
  var outputAb = null;
  var outputOffset = 0;

  return promise.resolve().then(function() {
    stream = z_stream.malloc().cast(z_stream_p);
    memset(stream, 0, z_stream.sizeof());

    output = malloc(bufferSize);
    var result = deflateInit(stream, level);
    return nacl.commitPromise(result);
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
      var inputSliceAb = sliceArrayBuffer(inputAb, inputOffset,
                                          inputOffsetEnd);
      var inputSlice = arrayBufferMap(inputSliceAb);
      z_stream.fields.next_in.set(stream, inputSlice.cast(uint8_p));
      z_stream.fields.avail_in.set(stream, inputSliceAb.byteLength);
      z_stream.fields.next_out.set(stream, output.cast(uint8_p));
      z_stream.fields.avail_out.set(stream, bufferSize);
    }

    var inputLeft = inputAb.byteLength - inputOffset;
    var flush = inputLeft < bufferSize ? Z_FINISH : Z_NO_FLUSH;
    var preAvailIn = z_stream.fields.avail_in.get(stream);
    var result = deflate(stream, flush);
    var availIn = z_stream.fields.avail_in.get(stream);
    var availOut = z_stream.fields.avail_out.get(stream);
    var outUsed = sub(bufferSize, availOut);
    var compressedAb = arrayBufferCreate(outUsed);
    var outputAbPtr = arrayBufferMap(compressedAb);
    memcpy(outputAbPtr, output, outUsed);
    return nacl.commitPromise(result, availIn, availOut, preAvailIn,
                              compressedAb);
  }).then(function(result, availIn, availOut, preAvailIn, compressedAb) {
    // Consume the final bit of output, if any.
    if (compressedAb) {
      outputAb = copyToArrayBuffer(outputAb, outputOffset, compressedAb);
      outputOffset += compressedAb.byteLength;
    }

    return promise.resolve(outputAb);
  }).finally(function() {
    free(stream);
    free(output);
    return nacl.commitPromise();
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
