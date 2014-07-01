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

define([], function() {

  function init(module) {
    var m = module;
    var t = m.types;

    m.typeBuilder_.makeVoidType(1, 'void');
    m.typeBuilder_.makePrimitiveType(2, 'char', 1, true, true);
    m.typeBuilder_.makePrimitiveType(3, 'int8', 1, true, true);
    m.typeBuilder_.makePrimitiveType(4, 'uint8', 1, true, false);
    m.typeBuilder_.makePrimitiveType(5, 'int16', 2, true, true);
    m.typeBuilder_.makePrimitiveType(6, 'uint16', 2, true, false);
    m.typeBuilder_.makePrimitiveType(7, 'int32', 4, true, true);
    m.typeBuilder_.makePrimitiveType(8, 'uint32', 4, true, false);
    m.typeBuilder_.makePrimitiveType(9, 'long', 4, true, true);
    m.typeBuilder_.makePrimitiveType(10, 'ulong', 4, true, false);
    m.typeBuilder_.makePrimitiveType(11, 'int64', 8, true, true);
    m.typeBuilder_.makePrimitiveType(12, 'uint64', 8, true, false);
    m.typeBuilder_.makePrimitiveType(13, 'float32', 4, false, false);
    m.typeBuilder_.makePrimitiveType(14, 'float64', 8, false, false);
    m.makeAliasType('uchar', t.uint8);
    m.makeAliasType('short', t.int16);
    m.makeAliasType('ushort', t.uint16);
    m.makeAliasType('int', t.int32);
    m.makeAliasType('uint', t.uint32);
    m.makeAliasType('longlong', t.int64);
    m.makeAliasType('ulonglong', t.uint64);
    m.makeAliasType('float', t.float32);
    m.makeAliasType('double', t.float64);
    m.makeAliasType('size_t', t.uint32);
    m.makeAliasType('ssize_t', t.int32);
    m.makeAliasType('off_t', t.int64);
    m.makePointerType(15, 'void$', t.void);
    m.makePointerType(16, 'char$', t.char);
    m.makePointerType(17, 'int8$', t.int8);
    m.makePointerType(18, 'uint8$', t.uint8);
    m.makePointerType(19, 'int16$', t.int16);
    m.makePointerType(20, 'uint16$', t.uint16);
    m.makePointerType(21, 'int32$', t.int32);
    m.makePointerType(22, 'uint32$', t.uint32);
    m.makePointerType(23, 'long$', t.long);
    m.makePointerType(24, 'ulong$', t.ulong);
    m.makePointerType(25, 'int64$', t.int64);
    m.makePointerType(26, 'uint64$', t.uint64);
    m.makePointerType(27, 'float32$', t.float32);
    m.makePointerType(28, 'float64$', t.float64);
    m.makePointerType(29, 'void$$', t.void$);
    m.typeBuilder_.makePepperType(30, 'Var', undefined);
    m.typeBuilder_.makePepperType(31, 'ArrayBuffer', ArrayBuffer);
    m.typeBuilder_.makePepperType(32, 'Array', Array);
    m.typeBuilder_.makePepperType(33, 'Dictionary', Object);
    m.typeBuilder_.makePepperType(34, 'String', String);

    var fnType_getVoidP = m.makeFunctionType(35, t.void$, t.void$$);
    var fnType_getChar = m.makeFunctionType(36, t.char, t.char$);
    var fnType_getInt8 = m.makeFunctionType(37, t.int8, t.int8$);
    var fnType_getUint8 = m.makeFunctionType(38, t.uint8, t.uint8$);
    var fnType_getInt16 = m.makeFunctionType(39, t.int16, t.int16$);
    var fnType_getUint16 = m.makeFunctionType(40, t.uint16, t.uint16$);
    var fnType_getInt32 = m.makeFunctionType(41, t.int32, t.int32$);
    var fnType_getUint32 = m.makeFunctionType(42, t.uint32, t.uint32$);
    var fnType_getLong = m.makeFunctionType(43, t.long, t.long$);
    var fnType_getUlong = m.makeFunctionType(44, t.ulong, t.ulong$);
    var fnType_getInt64 = m.makeFunctionType(45, t.int64, t.int64$);
    var fnType_getUint64 = m.makeFunctionType(46, t.uint64, t.uint64$);
    var fnType_getFloat32 = m.makeFunctionType(47, t.float32, t.float32$);
    var fnType_getFloat64 = m.makeFunctionType(48, t.float64, t.float64$);
    var fnType_setVoidP = m.makeFunctionType(49, t.void, t.void$$, t.void$);
    var fnType_setChar = m.makeFunctionType(50, t.void, t.char$, t.char);
    var fnType_setInt8 = m.makeFunctionType(51, t.void, t.int8$, t.int8);
    var fnType_setUint8 = m.makeFunctionType(52, t.void, t.uint8$, t.uint8);
    var fnType_setInt16 = m.makeFunctionType(53, t.void, t.int16$, t.int16);
    var fnType_setUint16 = m.makeFunctionType(54, t.void, t.uint16$, t.uint16);
    var fnType_setInt32 = m.makeFunctionType(55, t.void, t.int32$, t.int32);
    var fnType_setUint32 = m.makeFunctionType(56, t.void, t.uint32$, t.uint32);
    var fnType_setLong = m.makeFunctionType(57, t.void, t.long$, t.long);
    var fnType_setUlong = m.makeFunctionType(58, t.void, t.ulong$, t.ulong);
    var fnType_setInt64 = m.makeFunctionType(59, t.void, t.int64$, t.int64);
    var fnType_setUint64 = m.makeFunctionType(60, t.void, t.uint64$, t.uint64);
    var fnType_setFloat32 = m.makeFunctionType(61, t.void, t.float32$, t.float32);
    var fnType_setFloat64 = m.makeFunctionType(62, t.void, t.float64$, t.float64);
    var fnType_addVoidP = m.makeFunctionType(63, t.void$, t.void$, t.int32);
    var fnType_addInt32 = m.makeFunctionType(64, t.int32, t.int32, t.int32);
    var fnType_addUint32 = m.makeFunctionType(65, t.uint32, t.uint32, t.uint32);
    var fnType_addInt64 = m.makeFunctionType(66, t.int64, t.int64, t.int64);
    var fnType_addUint64 = m.makeFunctionType(67, t.uint64, t.uint64, t.uint64);
    var fnType_addFloat32 = m.makeFunctionType(68, t.float32, t.float32, t.float32);
    var fnType_addFloat64 = m.makeFunctionType(69, t.float64, t.float64, t.float64);
    var fnType_subVoidP = fnType_addVoidP;
    var fnType_subInt32 = fnType_addInt32;
    var fnType_subUint32 = fnType_addUint32;
    var fnType_subInt64 = fnType_addInt64;
    var fnType_subUint64 = fnType_addUint64;
    var fnType_subFloat32 = fnType_addFloat32;
    var fnType_subFloat64 = fnType_addFloat64;
    var fnType_free = m.makeFunctionType(70, t.void, t.void$);
    var fnType_malloc = m.makeFunctionType(71, t.void$, t.size_t);
    var fnType_memset = m.makeFunctionType(72, t.void$, t.void$, t.int, t.size_t);
    var fnType_memcpy = m.makeFunctionType(73, t.void$, t.void$, t.void$, t.size_t);
    var fnType_strlen = m.makeFunctionType(74, t.size_t, t.char$);
    var fnType_puts = m.makeFunctionType(75, t.int, t.char$);
    var fnType_varAddRef = m.makeFunctionType(76, t.void, t.Var);
    var fnType_varRelease = fnType_varAddRef;
    var fnType_varFromUtf8 = m.makeFunctionType(77, t.String, t.char$, t.uint32);
    var fnType_varToUtf8 = m.makeFunctionType(78, t.char$, t.String, t.uint32$);
    var fnType_arrayCreate = m.makeFunctionType(79, t.Array);
    var fnType_arrayGet = m.makeFunctionType(80, t.Var, t.Array, t.uint32);
    var fnType_arraySet = m.makeFunctionType(81, t.int32, t.Array, t.uint32, t.Var);
    var fnType_arrayGetLength = m.makeFunctionType(82, t.uint32, t.Array);
    var fnType_arraySetLength = m.makeFunctionType(83, t.int32, t.Array, t.uint32);
    var fnType_arrayBufferCreate = m.makeFunctionType(84, t.ArrayBuffer, t.uint32);
    var fnType_arrayBufferByteLength = m.makeFunctionType(85, t.int32, t.ArrayBuffer, t.uint32$);
    var fnType_arrayBufferMap = m.makeFunctionType(86, t.void$, t.ArrayBuffer);
    var fnType_arrayBufferUnmap = m.makeFunctionType(87, t.void, t.ArrayBuffer);
    var fnType_dictCreate = m.makeFunctionType(88, t.Dictionary);
    var fnType_dictGet = m.makeFunctionType(89, t.Var, t.Dictionary, t.Var);
    var fnType_dictSet = m.makeFunctionType(90, t.int32, t.Dictionary, t.Var, t.Var);
    var fnType_dictDelete = m.makeFunctionType(91, t.void, t.Dictionary, t.Var);
    var fnType_dictHasKey = m.makeFunctionType(92, t.int32, t.Dictionary, t.Var);

    m.makeFunction('get', [
      fnType_getVoidP,
      fnType_getChar,
      fnType_getInt8,
      fnType_getUint8,
      fnType_getInt16,
      fnType_getUint16,
      fnType_getInt32,
      fnType_getUint32,
      fnType_getLong,
      fnType_getUlong,
      fnType_getInt64,
      fnType_getUint64,
      fnType_getFloat32,
      fnType_getFloat64,
    ]);
    m.makeFunction('set', [
      fnType_setVoidP,
      fnType_setChar,
      fnType_setInt8,
      fnType_setUint8,
      fnType_setInt16,
      fnType_setUint16,
      fnType_setInt32,
      fnType_setUint32,
      fnType_setLong,
      fnType_setUlong,
      fnType_setInt64,
      fnType_setUint64,
      fnType_setFloat32,
      fnType_setFloat64,
    ]);
    m.makeFunction('add', [
      fnType_addVoidP,
      fnType_addInt32,
      fnType_addUint32,
      fnType_addInt64,
      fnType_addUint64,
      fnType_addFloat32,
      fnType_addFloat64,
    ]);
    m.makeFunction('sub', [
      fnType_subVoidP,
      fnType_subInt32,
      fnType_subUint32,
      fnType_subInt64,
      fnType_subUint64,
      fnType_subFloat32,
      fnType_subFloat64,
    ]);
    m.makeFunction('free', fnType_free);
    m.makeFunction('malloc', fnType_malloc);
    m.makeFunction('memset', fnType_memset);
    m.makeFunction('memcpy', fnType_memcpy);
    m.makeFunction('strlen', fnType_strlen);
    m.makeFunction('puts', fnType_puts);
    m.makeFunction('varAddRef', fnType_varAddRef);
    m.makeFunction('varRelease', fnType_varRelease);
    m.makeFunction('varFromUtf8', fnType_varFromUtf8);
    m.makeFunction('varToUtf8', fnType_varToUtf8);
    m.makeFunction('arrayCreate', fnType_arrayCreate);
    m.makeFunction('arrayGet', fnType_arrayGet);
    m.makeFunction('arraySet', fnType_arraySet);
    m.makeFunction('arrayGetLength', fnType_arrayGetLength);
    m.makeFunction('arraySetLength', fnType_arraySetLength);
    m.makeFunction('arrayBufferCreate', fnType_arrayBufferCreate);
    m.makeFunction('arrayBufferByteLength', fnType_arrayBufferByteLength);
    m.makeFunction('arrayBufferMap', fnType_arrayBufferMap);
    m.makeFunction('arrayBufferUnmap', fnType_arrayBufferUnmap);
    m.makeFunction('dictCreate', fnType_dictCreate);
    m.makeFunction('dictGet', fnType_dictGet);
    m.makeFunction('dictSet', fnType_dictSet);
    m.makeFunction('dictDelete', fnType_dictDelete);
    m.makeFunction('dictHasKey', fnType_dictHasKey);
  }

  return {
    init: init
  };
});

