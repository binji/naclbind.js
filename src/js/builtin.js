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

// DO NOT EDIT, this file is auto-generated from //templates/builtin.js


"use strict";

function init(module) {
  var m = module;
  var t = m.types;

  m.typeBuilder_.makeVoidType(1, 'void');
  m.typeBuilder_.makePrimitiveType(2, 'char', 1, true, true);
  m.typeBuilder_.makePrimitiveType(3, 'int8', 1, true, true);
  m.typeBuilder_.makePrimitiveType(4, 'uint8', 1, false, true);
  m.typeBuilder_.makePrimitiveType(5, 'int16', 2, true, true);
  m.typeBuilder_.makePrimitiveType(6, 'uint16', 2, false, true);
  m.typeBuilder_.makePrimitiveType(7, 'int32', 4, true, true);
  m.typeBuilder_.makePrimitiveType(8, 'uint32', 4, false, true);
  m.typeBuilder_.makePrimitiveType(9, 'long', 4, true, true);
  m.typeBuilder_.makePrimitiveType(10, 'ulong', 4, false, true);
  m.typeBuilder_.makePrimitiveType(11, 'int64', 8, true, true);
  m.typeBuilder_.makePrimitiveType(12, 'uint64', 8, false, true);
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
  var fnType_ltVoidP = m.makeFunctionType(70, t.int, t.void$, t.void$);
  var fnType_ltChar = m.makeFunctionType(71, t.int, t.char, t.char);
  var fnType_ltInt8 = m.makeFunctionType(72, t.int, t.int8, t.int8);
  var fnType_ltUint8 = m.makeFunctionType(73, t.int, t.uint8, t.uint8);
  var fnType_ltInt16 = m.makeFunctionType(74, t.int, t.int16, t.int16);
  var fnType_ltUint16 = m.makeFunctionType(75, t.int, t.uint16, t.uint16);
  var fnType_ltInt32 = fnType_addInt32;
  var fnType_ltUint32 = m.makeFunctionType(76, t.int, t.uint32, t.uint32);
  var fnType_ltLong = m.makeFunctionType(77, t.int, t.long, t.long);
  var fnType_ltUlong = m.makeFunctionType(78, t.int, t.ulong, t.ulong);
  var fnType_ltInt64 = m.makeFunctionType(79, t.int, t.int64, t.int64);
  var fnType_ltUint64 = m.makeFunctionType(80, t.int, t.uint64, t.uint64);
  var fnType_ltFloat32 = m.makeFunctionType(81, t.int, t.float32, t.float32);
  var fnType_ltFloat64 = m.makeFunctionType(82, t.int, t.float64, t.float64);
  var fnType_lteVoidP = fnType_ltVoidP;
  var fnType_lteChar = fnType_ltChar;
  var fnType_lteInt8 = fnType_ltInt8;
  var fnType_lteUint8 = fnType_ltUint8;
  var fnType_lteInt16 = fnType_ltInt16;
  var fnType_lteUint16 = fnType_ltUint16;
  var fnType_lteInt32 = fnType_addInt32;
  var fnType_lteUint32 = fnType_ltUint32;
  var fnType_lteLong = fnType_ltLong;
  var fnType_lteUlong = fnType_ltUlong;
  var fnType_lteInt64 = fnType_ltInt64;
  var fnType_lteUint64 = fnType_ltUint64;
  var fnType_lteFloat32 = fnType_ltFloat32;
  var fnType_lteFloat64 = fnType_ltFloat64;
  var fnType_gtVoidP = fnType_ltVoidP;
  var fnType_gtChar = fnType_ltChar;
  var fnType_gtInt8 = fnType_ltInt8;
  var fnType_gtUint8 = fnType_ltUint8;
  var fnType_gtInt16 = fnType_ltInt16;
  var fnType_gtUint16 = fnType_ltUint16;
  var fnType_gtInt32 = fnType_addInt32;
  var fnType_gtUint32 = fnType_ltUint32;
  var fnType_gtLong = fnType_ltLong;
  var fnType_gtUlong = fnType_ltUlong;
  var fnType_gtInt64 = fnType_ltInt64;
  var fnType_gtUint64 = fnType_ltUint64;
  var fnType_gtFloat32 = fnType_ltFloat32;
  var fnType_gtFloat64 = fnType_ltFloat64;
  var fnType_gteVoidP = fnType_ltVoidP;
  var fnType_gteChar = fnType_ltChar;
  var fnType_gteInt8 = fnType_ltInt8;
  var fnType_gteUint8 = fnType_ltUint8;
  var fnType_gteInt16 = fnType_ltInt16;
  var fnType_gteUint16 = fnType_ltUint16;
  var fnType_gteInt32 = fnType_addInt32;
  var fnType_gteUint32 = fnType_ltUint32;
  var fnType_gteLong = fnType_ltLong;
  var fnType_gteUlong = fnType_ltUlong;
  var fnType_gteInt64 = fnType_ltInt64;
  var fnType_gteUint64 = fnType_ltUint64;
  var fnType_gteFloat32 = fnType_ltFloat32;
  var fnType_gteFloat64 = fnType_ltFloat64;
  var fnType_eqVoidP = fnType_ltVoidP;
  var fnType_eqChar = fnType_ltChar;
  var fnType_eqInt8 = fnType_ltInt8;
  var fnType_eqUint8 = fnType_ltUint8;
  var fnType_eqInt16 = fnType_ltInt16;
  var fnType_eqUint16 = fnType_ltUint16;
  var fnType_eqInt32 = fnType_addInt32;
  var fnType_eqUint32 = fnType_ltUint32;
  var fnType_eqLong = fnType_ltLong;
  var fnType_eqUlong = fnType_ltUlong;
  var fnType_eqInt64 = fnType_ltInt64;
  var fnType_eqUint64 = fnType_ltUint64;
  var fnType_eqFloat32 = fnType_ltFloat32;
  var fnType_eqFloat64 = fnType_ltFloat64;
  var fnType_neVoidP = fnType_ltVoidP;
  var fnType_neChar = fnType_ltChar;
  var fnType_neInt8 = fnType_ltInt8;
  var fnType_neUint8 = fnType_ltUint8;
  var fnType_neInt16 = fnType_ltInt16;
  var fnType_neUint16 = fnType_ltUint16;
  var fnType_neInt32 = fnType_addInt32;
  var fnType_neUint32 = fnType_ltUint32;
  var fnType_neLong = fnType_ltLong;
  var fnType_neUlong = fnType_ltUlong;
  var fnType_neInt64 = fnType_ltInt64;
  var fnType_neUint64 = fnType_ltUint64;
  var fnType_neFloat32 = fnType_ltFloat32;
  var fnType_neFloat64 = fnType_ltFloat64;
  var fnType_free = m.makeFunctionType(83, t.void, t.void$);
  var fnType_malloc = m.makeFunctionType(84, t.void$, t.size_t);
  var fnType_memset = m.makeFunctionType(85, t.void$, t.void$, t.int, t.size_t);
  var fnType_memcpy = m.makeFunctionType(86, t.void$, t.void$, t.void$, t.size_t);
  var fnType_strlen = m.makeFunctionType(87, t.size_t, t.char$);
  var fnType_puts = m.makeFunctionType(88, t.int, t.char$);
  var fnType_varAddRef = m.makeFunctionType(89, t.void, t.Var);
  var fnType_varRelease = fnType_varAddRef;
  var fnType_varFromUtf8 = m.makeFunctionType(90, t.String, t.char$, t.uint32);
  var fnType_varToUtf8 = m.makeFunctionType(91, t.char$, t.String, t.uint32$);
  var fnType_arrayCreate = m.makeFunctionType(92, t.Array);
  var fnType_arrayGet = m.makeFunctionType(93, t.Var, t.Array, t.uint32);
  var fnType_arraySet = m.makeFunctionType(94, t.int32, t.Array, t.uint32, t.Var);
  var fnType_arrayGetLength = m.makeFunctionType(95, t.uint32, t.Array);
  var fnType_arraySetLength = m.makeFunctionType(96, t.int32, t.Array, t.uint32);
  var fnType_arrayBufferCreate = m.makeFunctionType(97, t.ArrayBuffer, t.uint32);
  var fnType_arrayBufferByteLength = m.makeFunctionType(98, t.int32, t.ArrayBuffer, t.uint32$);
  var fnType_arrayBufferMap = m.makeFunctionType(99, t.void$, t.ArrayBuffer);
  var fnType_arrayBufferUnmap = m.makeFunctionType(100, t.void, t.ArrayBuffer);
  var fnType_dictCreate = m.makeFunctionType(101, t.Dictionary);
  var fnType_dictGet = m.makeFunctionType(102, t.Var, t.Dictionary, t.Var);
  var fnType_dictSet = m.makeFunctionType(103, t.int32, t.Dictionary, t.Var, t.Var);
  var fnType_dictDelete = m.makeFunctionType(104, t.void, t.Dictionary, t.Var);
  var fnType_dictHasKey = m.makeFunctionType(105, t.int32, t.Dictionary, t.Var);

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
  m.makeFunction('lt', [
    fnType_ltVoidP,
    fnType_ltChar,
    fnType_ltInt8,
    fnType_ltUint8,
    fnType_ltInt16,
    fnType_ltUint16,
    fnType_ltInt32,
    fnType_ltUint32,
    fnType_ltLong,
    fnType_ltUlong,
    fnType_ltInt64,
    fnType_ltUint64,
    fnType_ltFloat32,
    fnType_ltFloat64,
  ]);
  m.makeFunction('lte', [
    fnType_lteVoidP,
    fnType_lteChar,
    fnType_lteInt8,
    fnType_lteUint8,
    fnType_lteInt16,
    fnType_lteUint16,
    fnType_lteInt32,
    fnType_lteUint32,
    fnType_lteLong,
    fnType_lteUlong,
    fnType_lteInt64,
    fnType_lteUint64,
    fnType_lteFloat32,
    fnType_lteFloat64,
  ]);
  m.makeFunction('gt', [
    fnType_gtVoidP,
    fnType_gtChar,
    fnType_gtInt8,
    fnType_gtUint8,
    fnType_gtInt16,
    fnType_gtUint16,
    fnType_gtInt32,
    fnType_gtUint32,
    fnType_gtLong,
    fnType_gtUlong,
    fnType_gtInt64,
    fnType_gtUint64,
    fnType_gtFloat32,
    fnType_gtFloat64,
  ]);
  m.makeFunction('gte', [
    fnType_gteVoidP,
    fnType_gteChar,
    fnType_gteInt8,
    fnType_gteUint8,
    fnType_gteInt16,
    fnType_gteUint16,
    fnType_gteInt32,
    fnType_gteUint32,
    fnType_gteLong,
    fnType_gteUlong,
    fnType_gteInt64,
    fnType_gteUint64,
    fnType_gteFloat32,
    fnType_gteFloat64,
  ]);
  m.makeFunction('eq', [
    fnType_eqVoidP,
    fnType_eqChar,
    fnType_eqInt8,
    fnType_eqUint8,
    fnType_eqInt16,
    fnType_eqUint16,
    fnType_eqInt32,
    fnType_eqUint32,
    fnType_eqLong,
    fnType_eqUlong,
    fnType_eqInt64,
    fnType_eqUint64,
    fnType_eqFloat32,
    fnType_eqFloat64,
  ]);
  m.makeFunction('ne', [
    fnType_neVoidP,
    fnType_neChar,
    fnType_neInt8,
    fnType_neUint8,
    fnType_neInt16,
    fnType_neUint16,
    fnType_neInt32,
    fnType_neUint32,
    fnType_neLong,
    fnType_neUlong,
    fnType_neInt64,
    fnType_neUint64,
    fnType_neFloat32,
    fnType_neFloat64,
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

module.exports = {
  init: init
};

