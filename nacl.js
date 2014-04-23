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

define(['promise'], function(promise) {

  function assert(cond, msg) {
    if (!cond) {
      throw new Error('Assertion failed' + (msg ? (': ' + msg) : '.'));
    }
  }

  function makeModule(name, nmf, mimeType) {
    return new Module(name, nmf, mimeType);
  }

  // Module ////////////////////////////////////////////////////////////////////
  function Module(name, nmf, mimeType) {
    this.name = name;
    this.nmf = nmf;
    this.mimeType = mimeType;
    this.element = null;

    this.loaded_ = false;
    this.nextCallbackId_ = 1;
    this.idCallbackMap_ = [];
    this.queuedMessages_ = [];
    this.commands_ = [];
    this.typeBuilder_ = new TypeBuilder();
    this.functionBuilder_ = new FunctionBuilder();
    this.handles_ = new HandleList();

    this.types = this.typeBuilder_.getNameHash();
    this.functions = this.functionBuilder_.getNameHash();

    this.initDefaults_();
    this.createEmbed_();
  }

  Module.prototype.createEmbed_ = function() {
    this.element = document.createElement('embed');
    this.element.setAttribute('width', '0');
    this.element.setAttribute('height', '0');
    this.element.setAttribute('src', this.nmf);
    this.element.setAttribute('type', this.mimeType);

    this.element.addEventListener('load', this.onLoad_.bind(this), false);
    this.element.addEventListener('message', this.onMessage_.bind(this), false);
    this.element.addEventListener('error', this.onError_.bind(this), false);
    this.element.addEventListener('crash', this.onCrash_.bind(this), false);
    document.body.appendChild(this.element);
  };

  Module.prototype.postMessage = function(msg, callback) {
    var id = this.nextCallbackId_++;
    this.idCallbackMap_[id] = callback;

    msg.id = id;
    if (this.loaded_) {
      this.element.postMessage(msg);
    } else {
      this.queuedMessages_.push(msg);
    }
    return id;
  };

  Module.prototype.makeContext = function() {
    return new Context(this);
  };

  Module.prototype.onLoad_ = function(event) {
    console.log('module loaded.');
    this.loaded_ = true;
    this.postQueuedMessages_();
  };

  Module.prototype.onMessage_ = function(event) {
    var msg = event.data;
    if (typeof(msg) !== 'object') {
      var msg = this.name_ + ': unexpected value from module: ' +
          JSON.stringify(msg);
      throw new Error(msg);
    }

    if (msg.msg) {
      console.log(msg.msg);
      return;
    }

    var id = msg.id;
    if (id !== 0) {
      var callback = this.idCallbackMap_[id];
      callback(msg);
      delete this.idCallbackMap_[id];
    }
  };

  Module.prototype.onError_ = function(event) {
    var msg = this.name_ + ': error loading NaCl module: ' + this.element.lastError;
    throw new Error(msg);
  };

  Module.prototype.onCrash_ = function(event) {
    var msg = this.name_ + ': NaCl module crashed: ' + this.element.exitStatus;
    throw new Error(msg);
  };

  Module.prototype.postQueuedMessages_ = function() {
    var that = this;
    this.queuedMessages_.forEach(function(msg) {
      that.element.postMessage(msg);
    });

    this.queuedMessages_ = null;
  };

  Module.prototype.callFunction = function(context, func, args) {
    assert(func instanceof CFunction, 'expected func to be CFunction');

    var funcType = this.findOverload_(func.name, args, func.overloads);
    assert(funcType !== null);

    var retHandle = null;
    var retHandleId = 0;
    if (funcType.retType !== this.types.void) {
      retHandle = this.handles_.makeHandle(context, funcType.retType);
      retHandleId = retHandle.id;
    }

    var message = {
      cmd: func.name,
      type: funcType.id,
      args: [],
      argIsHandle: [],
      ret: retHandleId
    };
    for (var i = 0; i < funcType.argTypes.length; ++i) {
      var arg = args[i];
      var value;
      if (arg instanceof Handle) {
        value = arg.id;
      } else {
        value = arg;
      }
      message.args.push(value);
      message.argIsHandle.push(arg instanceof Handle);
    }

    this.commands_.push(message);

    return retHandle;
  };


  Module.prototype.commit = function() {
    assert(arguments.length >= 1, 'expected callback.');

    var callback = arguments[arguments.length - 1];
    assert(callback instanceof Function, 'callback is not Function.');

    var args = Array.prototype.slice.call(arguments, 0, -1);

    var serializeHandle = function(handle) {
      assert(handle instanceof Handle, 'handle is not a Handle.');
      return handle.id;
    };

    var handles = args.map(serializeHandle);
    var msg = {
      msgs: this.commands_,  // TODO(binji): rename "msgs"
      handles: handles,
    };

    // Remove committed commands.
    this.commands_ = [];

    this.postMessage(msg, function(result) {
      function idToHandle(value, ix) {
        if (typeof(value) === 'number' && !args[ix].type.isPrimitive()) {
          return this.handles_.getHandle(value);
        }
        return value;
      };

      // convert back from Ids to Handles; keep the primitive values the same.
      var handles = Array.prototype.map.call(result.values, idToHandle);
      callback.apply(null, handles);
    });
  };

  Module.prototype.commitPromise = function() {
    var that = this;
    var args = Array.prototype.slice.call(arguments);
    return new promise.PromisePlus(function(resolve, reject, resolveMany) {
      args.push(function() {
        resolveMany.apply(null, arguments);
      });
      that.commit.apply(that, args);
    });
  };

  Module.prototype.initDefaults_ = function() {
    var t = this.types;

    this.typeBuilder_.makeVoidType(1);
    this.typeBuilder_.makePrimitiveType(2, 'int8', 1, true, true);
    this.typeBuilder_.makePrimitiveType(3, 'uint8', 1, false, true);
    this.typeBuilder_.makePrimitiveType(4, 'int16', 2, true, true);
    this.typeBuilder_.makePrimitiveType(5, 'uint16', 2, false, true);
    this.typeBuilder_.makePrimitiveType(6, 'int32', 4, true, true);
    this.typeBuilder_.makePrimitiveType(7, 'uint32', 4, false, true);
    this.typeBuilder_.makePrimitiveType(8, 'int64', 8, true, true);
    this.typeBuilder_.makePrimitiveType(9, 'uint64', 8, false, true);
    this.typeBuilder_.makePrimitiveType(10, 'float32', 4, false, false);
    this.typeBuilder_.makePrimitiveType(11, 'float64', 8, false, false);

    this.makePointerType(12, t.void);
    this.makePointerType(13, t.int8);
    this.makePointerType(14, t.uint8);
    this.makePointerType(15, t.int16);
    this.makePointerType(16, t.uint16);
    this.makePointerType(17, t.int32);
    this.makePointerType(18, t.uint32);
    this.makePointerType(19, t.int64);
    this.makePointerType(20, t.uint64);
    this.makePointerType(21, t.float32);
    this.makePointerType(22, t.float64);
    this.makePointerType(23, t.void$);

    this.typeBuilder_.makePepperType(24, 'Var', undefined);
    this.typeBuilder_.makePepperType(25, 'ArrayBuffer', ArrayBuffer);
    this.typeBuilder_.makePepperType(26, 'Array', Array);
    this.typeBuilder_.makePepperType(27, 'Dictionary', Object);
    this.typeBuilder_.makePepperType(28, 'String', String);

    var getTypes = [
      this.makeFunctionType(29, t.void$, t.void$$),
      this.makeFunctionType(30, t.int8, t.int8$),
      this.makeFunctionType(31, t.uint8, t.uint8$),
      this.makeFunctionType(32, t.int16, t.int16$),
      this.makeFunctionType(33, t.uint16, t.uint16$),
      this.makeFunctionType(34, t.int32, t.int32$),
      this.makeFunctionType(35, t.uint32, t.uint32$),
      this.makeFunctionType(36, t.int64, t.int64$),
      this.makeFunctionType(37, t.uint64, t.uint64$),
      this.makeFunctionType(38, t.float32, t.float32$),
      this.makeFunctionType(39, t.float64, t.float64$),
    ];

    var setTypes = [
      this.makeFunctionType(40, t.void, t.void$$, t.void$),
      this.makeFunctionType(41, t.void, t.int8$, t.int8),
      this.makeFunctionType(42, t.void, t.uint8$, t.uint8),
      this.makeFunctionType(43, t.void, t.int16$, t.int16),
      this.makeFunctionType(44, t.void, t.uint16$, t.uint16),
      this.makeFunctionType(45, t.void, t.int32$, t.int32),
      this.makeFunctionType(46, t.void, t.uint32$, t.uint32),
      this.makeFunctionType(47, t.void, t.int64$, t.int64),
      this.makeFunctionType(48, t.void, t.uint64$, t.uint64),
      this.makeFunctionType(49, t.void, t.float32$, t.float32),
      this.makeFunctionType(50, t.void, t.float64$, t.float64),
    ];

    var freeType = this.makeFunctionType(51, t.void, t.void$);
    var mallocType = this.makeFunctionType(52, t.void$, t.uint32);
    var memsetType = this.makeFunctionType(53, t.void, t.void$, t.int32, t.uint32);
    var memcpyType = this.makeFunctionType(54, t.void, t.void$, t.void$, t.uint32);
    var strlenType = this.makeFunctionType(55, t.uint32, t.uint8$);

    var varAddRefReleaseType = this.makeFunctionType(56, t.void, t.Var);
    var varFromUtf8Type = this.makeFunctionType(57, t.Var, t.uint8$, t.uint32);
    var varToUtf8Type = this.makeFunctionType(58, t.uint8$, t.Var, t.uint32$);

    var arrayCreateType = this.makeFunctionType(59, t.Array);
    var arrayGetType = this.makeFunctionType(60, t.Var, t.Array, t.uint32);
    var arraySetType = this.makeFunctionType(61, t.int32, t.Array, t.uint32, t.Var);
    var arrayGetLengthType = this.makeFunctionType(62, t.uint32, t.Array);
    var arraySetLengthType = this.makeFunctionType(63, t.int32, t.Array, t.uint32);

    var arrayBufferCreateType = this.makeFunctionType(64, t.ArrayBuffer, t.uint32);
    var arrayBufferByteLengthType = this.makeFunctionType(65, t.int32, t.ArrayBuffer, t.uint32$);
    var arrayBufferMapType = this.makeFunctionType(66, t.void$, t.ArrayBuffer);
    var arrayBufferUnmapType = this.makeFunctionType(67, t.void, t.ArrayBuffer);

    var dictCreateType = this.makeFunctionType(68, t.Dictionary);
    var dictGetType = this.makeFunctionType(69, t.Var, t.Dictionary, t.Var);
    var dictSetType = this.makeFunctionType(70, t.int32, t.Dictionary, t.Var, t.Var);
    var dictDeleteType = this.makeFunctionType(71, t.void, t.Dictionary, t.Var);
    var dictHasKeyType = this.makeFunctionType(72, t.int32, t.Dictionary, t.Var);

    var binopVoid$IntType = this.makeFunctionType(73, t.void$, t.void$, t.int32);
    var binopInt32Type = this.makeFunctionType(74, t.int32, t.int32, t.int32);
    var binopUint32Type = this.makeFunctionType(75, t.uint32, t.uint32, t.uint32);
    var binopInt64Type = this.makeFunctionType(76, t.int64, t.int64, t.int64);
    var binopUint64Type = this.makeFunctionType(77, t.uint64, t.uint64, t.uint64);
    var binopFloatType = this.makeFunctionType(78, t.float32, t.float32, t.float32);
    var binopDoubleType = this.makeFunctionType(79, t.float64, t.float64, t.float64);

    var addSubTypes = [
      binopVoid$IntType, binopInt32Type, binopUint32Type, binopInt64Type,
      binopUint64Type, binopFloatType, binopDoubleType,
    ];

    // builtin functions
    this.makeFunction('get', getTypes);
    this.makeFunction('set', setTypes);
    this.makeFunction('add', addSubTypes);
    this.makeFunction('sub', addSubTypes);

    // stdlib
    this.makeFunction('free', freeType);
    this.makeFunction('malloc', mallocType);
    this.makeFunction('memcpy', memcpyType);
    this.makeFunction('memset', memsetType);
    this.makeFunction('strlen', strlenType);

    // PPB_Var
    this.makeFunction('varAddRef', varAddRefReleaseType);
    this.makeFunction('varRelease', varAddRefReleaseType);
    this.makeFunction('varFromUtf8', varFromUtf8Type);
    this.makeFunction('varToUtf8', varToUtf8Type);

    // PPB_VarArray
    this.makeFunction('arrayCreate', arrayCreateType);
    this.makeFunction('arrayGet', arrayGetType);
    this.makeFunction('arraySet', arraySetType);
    this.makeFunction('arrayGetLength', arrayGetLengthType);
    this.makeFunction('arraySetLength', arraySetLengthType);

    // PPB_VarArrayBuffer
    this.makeFunction('arrayBufferCreate', arrayBufferCreateType);
    this.makeFunction('arrayBufferMap', arrayBufferMapType);
    this.makeFunction('arrayBufferUnmap', arrayBufferUnmapType);

    // PPB_VarDictionary
    this.makeFunction('dictCreate', dictCreateType);
    this.makeFunction('dictGet', dictGetType);
    this.makeFunction('dictSet', dictSetType);
    this.makeFunction('dictDelete', dictDeleteType);
    this.makeFunction('dictHasKey', dictHasKeyType);
    this.makeFunction('dictGetKeys', dictGetType);
  };

  Module.prototype.makePointerType = function(id, baseType) {
    return this.typeBuilder_.makePointerType(id, baseType);
  };

  Module.prototype.makeStructType = function(id, size, name, fields) {
    return this.typeBuilder_.makeStructType(id, size, name, fields);
  };

  Module.prototype.makeFunctionType = function(id, retType) {
    return this.typeBuilder_.makeFunctionType.apply(this.typeBuilder_, arguments);
  };

  Module.prototype.makeFunction = function(name, overloads) {
    return this.functionBuilder_.makeFunction(name, overloads);
  };

  // TODO(binji): Where should these go? On the context...?
  Module.prototype.setField = function(context, structField, struct_p, value) {
    var dst = this.functions.add(context, struct_p, structField.offset);
    var pointerType = this.typeBuilder_.getPointerType(structField.type);
    if (structField.type.isPointer()) {
      return this.functions.set(context, dst.cast(this.types.void$$), value.cast(this.types.void$));
    } else {
      return this.functions.set(context, dst.cast(pointerType), value);
    }
  };

  Module.prototype.getField = function(context, structField, struct_p) {
    var ptr = this.functions.add(context, struct_p, structField.offset);
    var pointerType = this.typeBuilder_.getPointerType(structField.type);
    if (structField.type.isPointer()) {
      return this.functions.get(context, ptr.cast(this.types.void$$)).cast(pointerType);
    } else {
      return this.functions.get(context, ptr.cast(pointerType));
    }
  };

  Module.prototype.mallocType = function(context, type) {
    var pointerType = this.typeBuilder_.getPointerType(type);
    return this.functions.malloc(context, type.sizeof()).cast(pointerType);
  };

  Module.prototype.findOverload_ = function(funcName, args, funcTypeList) {
    for (var i = 0; i < funcTypeList.length; ++i) {
      if (this.overloadMatches_(funcTypeList[i], args)) {
        return funcTypeList[i];
      }
    }

    // Display helpful warning.
    var msg;
    msg = 'No overload found for call "' + funcName + '(';
    msg += Array.prototype.join.call(args, ', ');
    msg += ')".\n';
    msg += "Possibilities:\n";
    for (var i = 0; i < funcTypeList.length; ++i) {
      msg += funcTypeList[i].toString() + "\n";
    }
    console.log(msg);

    return null;
  };

  Module.prototype.overloadMatches_ = function(funcType, args) {
    if (funcType.argTypes.length !== args.length) {
      return false;
    }

    for (var i = 0; i < funcType.argTypes.length; ++i) {
      var arg = args[i];
      var funcArgType = funcType.argTypes[i];
      var argType;
      if (arg instanceof Handle) {
        argType = arg.type;
      } else if (typeof(arg) === 'number') {
        if (Math.floor(arg) === arg) {
          // Int.
          argType = this.getIntType_(arg);
        } else {
          // Float.
          argType = this.types.float64;
        }
      } else if (arg instanceof ArrayBuffer) {
        argType = this.types.ArrayBuffer;
      } else {
        // TODO(binji): handle other pepper types.
        // What kind of type is this?
        console.log('Unexpected type of arg "' + arg + '": ' + typeof(arg));
        return false;
      }

      if (!this.canCoerceArgument_(argType, funcArgType)) {
        return false;
      }
    }
    return true;
  };

  Module.prototype.getIntType_ = function(value) {
    if (value >= -128 && value <= 127) {
      return this.types.int8;
    } else if (value >= -32768 && value <= 32767) {
      return this.types.int16;
    } else if (value >= -2147483648 && value <= 2147483647) {
      return this.types.int32;
    // TODO(binji): JavaScript numbers only have 53-bits of precision, so
    // this is not correct. We need a 64-bit int type.
    } else if (value >= -9223372036854775808 &&
               value <=  9223372036854775807) {
      return this.types.int64;
    } else {
      assert(value > 0, 'expected uint64. ' + value + ' <= 0.');
      return this.types.uint64;
    }
  };

  Module.prototype.canCoerceArgument_ = function(fromType, toType) {
    if (fromType === toType) {
      return true;
    }

    if (fromType.isPointer() && toType.isPointer() &&
        this.canCoercePointer_(fromType, toType)) {
      return true;
    }

    if (fromType.isPrimitive() && toType.isPrimitive() &&
        this.canCoercePrimitive_(fromType, toType)) {
      return true;
    }

    return false;
  };

  Module.prototype.canCoercePointer_ = function(fromType, toType) {
    assert(fromType.isPointer(), 'expected pointer, not ' + fromType);
    assert(toType.isPointer(), 'expected pointer, not ' + toType);
    // For now, we can only coerce pointers to void*. At some point, C++
    // inheritance could be supported as well.
    if (toType !== this.types.void$) {
      //console.log('Can only coerce to void*, not ' + toType + '.');
      return false;
    }
    return true;
  };

  Module.prototype.canCoercePrimitive_ = function(fromType, toType) {
    assert(fromType.isPrimitive(), 'expected primitive, not ' + fromType);
    assert(toType.isPrimitive(), 'expected primitive, not ' + toType);

    if (fromType.isInt() === toType.isInt()) {
      if (fromType.isInt()) {
        // Both ints.
        if (fromType.sizeof() > toType.sizeof()) {
          // console.log('Argument type is too large: ' + fromType + ' > ' + toType + '.');
          return false;
        } else if (fromType.sizeof() === toType.sizeof() &&
                   fromType.isSigned() !== toType.isSigned()) {
          // console.log('Signed/unsigned mismatch: ' + fromType + ', ' + toType + '.');
          return false;
        }
      } else {
        // Both floats.
        if (fromType.sizeof() > toType.sizeof()) {
          // console.log('Argument type is too large: ' + fromType + ' > ' + toType + '.');
          return false;
        }
      }
    } else {
      // One int, one float.
      if (fromType.isInt()) {
        // From int to float.
        if ((toType === this.types.float32 && fromType.sizeof() >= 4) ||
            (toType === this.types.float64 && fromType.sizeof() == 8)) {
          // console.log('Argument type is too large: ' + fromType + ' > ' + toType + '.');
          return false;
        }
      } else {
        // From float to int.
        // console.log('Implicit cast from float to int: ' + fromType + ' => ' + toType + '.');
        return false;
      }
    }

    return true;
  };

  // TODO(binji): better name?
  Module.prototype.destroyHandles = function(context) {
    // TODO(binji): share code with callFunction?
    var message = {
      cmd: '*destroyHandles',
      type: 0,  // None.
      args: [],
      argIsHandle: [],
      ret: 0,  // Invalid.
    };
    for (var i = 0; i < context.handles.length; ++i) {
      message.args.push(context.handles[i].id);
      message.argIsHandle.push(true);
    }

    this.commands_.push(message);

    // Remove them from the module's handle list too.
    this.handles_.destroyHandles(context.handles);
  };


  //// TypeBuilder /////////////////////////////////////////////////////////////
  function TypeBuilder() {
    this.idHash = {};
    this.nameHash = {};
  }

  TypeBuilder.prototype.registerType_ = function(id, newType) {
    assert(id !== 0, 'id !== 0');
    assert(!(id in this.idHash), 'id ' + id + ' already exists');
    assert(this.getTypeId(newType) === 0,
           'type ' + newType + ' already made with id ' + id);

    newType.id = id;
    this.idHash[id] = newType;
    this.nameHash[newType.getName()] = newType;
    return newType;
  };

  TypeBuilder.prototype.getNameHash = function() {
    return this.nameHash;
  };

  TypeBuilder.prototype.makeAliasType = function(id, name, type) {
    return this.registerType_(id, new AliasType(name, type));
  };

  TypeBuilder.prototype.makePepperType = function(id, name, type) {
    return this.registerType_(id, new PepperType(name, type));
  };

  TypeBuilder.prototype.makePointerType = function(id, baseType) {
    return this.registerType_(id, new PointerType(baseType));
  };

  TypeBuilder.prototype.makePrimitiveType = function(id, name, size, isSigned, isInt) {
    return this.registerType_(id, new PrimitiveType(name, size, isSigned, isInt));
  };

  TypeBuilder.prototype.makeStructType = function(id, name, size, fields) {
    return this.registerType_(id, new StructType(name, size, fields));
  };

  TypeBuilder.prototype.makeVoidType = function(id) {
    return this.registerType_(id, new VoidType(id));
  };

  TypeBuilder.prototype.makeFunctionType = function(id, retType) {
    var args = Array.prototype.slice.call(arguments, 1);
    var constructor = Function.bind.apply(FunctionType, [null].concat(args));
    return this.registerType_(id, new constructor());
  };

  TypeBuilder.prototype.getTypeId = function(type) {
    for (var id in this.idHash) {
      if (this.idHash[id].equals(type)) {
        return id;
      }
    }
    return 0;
  };

  TypeBuilder.prototype.getPointerType = function(baseType) {
    var newPointerType = new PointerType(baseType);
    var id = this.getTypeId(newPointerType);
    if (id === 0) {
      // Don't blow up yet... it is not an error to create a type that doesn't
      // have an id. We can't send it to the NaCl module, but we can use it for
      // type-checking.
      this.createStack = new Error().stack;
      return newPointerType;
      //throw new Error('Pointer type "' + pointerType + '" not defined.');
    }

    // Get the correct one.
    return this.idHash[id];
  };

  TypeBuilder.prototype.log = function() {
    for (var id in this.idHash) {
      if (this.idHash.hasOwnProperty(id)) {
        console.log('id: ' + id + ' type: ' + this.idHash[id]);
      }
    }
  };


  //// Context /////////////////////////////////////////////////////////////////
  function Context(module) {
    this.module = module;
    this.handles = [];
    // TODO(binji): nice way to clean up all these handles.
    // It would also be nice to clean up malloc'd memory, release PP_Vars, etc.
  }

  Context.prototype.registerHandle = function(handle) {
    assert(handle instanceof Handle, 'handle is not a Handle.');
    this.handles.push(handle);
  };

  Context.prototype.callFunction = function(func, args) {
    return this.module.callFunction(this, func, args);
  };


  //// HandleList //////////////////////////////////////////////////////////////
  function HandleList() {
    this.nextId_ = 1;
    this.handleIdHash_ = {};
  }

  HandleList.prototype.makeHandle = function(context, type, id) {
    return new Handle(this, context, type, id);
  };

  HandleList.prototype.getHandle = function(id) {
    return this.handleIdHash[id];
  };

  HandleList.prototype.registerHandle = function(handle) {
    var id = this.nextId_++;
    this.handleIdHash_[id] = handle;
    return id;
  };

  HandleList.prototype.destroyHandles = function(handles) {
    var that = this;
    handles.forEach(function(handle) {
      delete that.handleIdHash_[handle.id];
    });
  };


  //// Handle //////////////////////////////////////////////////////////////////
  function Handle(handleList, context, type, id) {
    this.handleList = handleList;
    this.type = type;
    if (id !== undefined) {
      this.id = id;
    } else {
      this.id = handleList.registerHandle(this);
      context.registerHandle(this);
    }

    this.context = context;
  }

  Handle.prototype.toString = function() {
    return '[Handle ' + this.id + ' ' + this.type.toString() + ']';
  };

  Handle.prototype.cast = function(newType) {
    // TODO(binji): check validity of cast
    return new Handle(this.handleList, this.context, newType, this.id);
  };


  //// Type ////////////////////////////////////////////////////////////////////
  function Type() { this.id = 0; }

  Type.prototype.equals = function(other) {
    return false;
  };

  Type.prototype.isPrimitive = function() {
    return this instanceof PrimitiveType;
  };

  Type.prototype.isPointer = function() {
    return this instanceof PointerType;
  };

  Type.prototype.isInt = function() { return false; };
  Type.prototype.isSigned = function() { return false; };

  Type.prototype.getName = function() { return null; };

  //// VoidType ////////////////////////////////////////////////////////////////
  function VoidType() { Type.call(this); }
  VoidType.prototype = new Type();
  VoidType.prototype.constructor = VoidType;
  VoidType.prototype.sizeof = function() { return 0; };
  VoidType.prototype.toString = function() { return 'void'; };
  VoidType.prototype.getName = function() { return 'void'; };

  VoidType.prototype.equals = function(other) {
    if (this === other) {
      return true;
    }

    return this.constructor === other.constructor;
  };


  //// PrimitiveType ///////////////////////////////////////////////////////////
  function PrimitiveType(name, size, isSigned, isInt) {
    Type.call(this);
    this.name = name;
    this.size = size;
    this.isSigned_ = isSigned;
    this.isInt_ = isInt;
  }

  PrimitiveType.prototype = new Type();
  PrimitiveType.prototype.constructor = PrimitiveType;
  PrimitiveType.prototype.sizeof = function() { return this.size; };
  PrimitiveType.prototype.toString = function() { return this.name; };
  PrimitiveType.prototype.getName = function() { return this.name; };
  PrimitiveType.prototype.isInt = function() { return this.isInt_; };
  PrimitiveType.prototype.isSigned = function() { return this.isSigned_; };

  PrimitiveType.prototype.equals = function(other) {
    if (this === other) {
      return true;
    }

    return this.constructor === other.constructor &&
           this.size === other.size &&
           this.name === other.name &&
           this.isInt_ === other.isInt_ &&
           this.isSigned_ === other.isSigned_;
  };


  //// PointerType /////////////////////////////////////////////////////////////
  function PointerType(baseType) {
    Type.call(this);
    this.baseType = baseType;
  }

  PointerType.prototype = new Type();
  PointerType.prototype.constructor = PointerType;

  PointerType.prototype.sizeof = function() {
    return 4;  // NaCl pointers are always 32-bit.
  };

  PointerType.prototype.toString = function() {
    return this.baseType.toString() + '*';
  };

  PointerType.prototype.getName = function() {
    // So it can be used as a JavaScript identifier.
    return this.baseType.getName() + '$';
  };

  PointerType.prototype.equals = function(other) {
    if (this === other) {
      return true;
    }

    return this.constructor === other.constructor &&
           this.baseType.equals(other.baseType);
  };


  //// StructField /////////////////////////////////////////////////////////////
  function StructField(name, type, offset) {
    this.name = name;
    this.type = type;
    this.offset = offset;
  }

  StructField.prototype.equals = function(other) {
    if (this === other) {
      return true;
    }

    return this.constructor === other.constructor &&
           this.name === other.name &&
           this.type === other.type &&
           this.offset === other.offset;
  };


  //// StructType //////////////////////////////////////////////////////////////
  function StructType(name, size, fields) {
    Type.call(this);
    this.name = name;
    this.size = size;
    this.fields = {};
    this.addFields_(fields);
  }

  StructType.prototype = new Type();
  StructType.prototype.constructor = StructType;

  StructType.prototype.sizeof = function() { return this.size; };
  StructType.prototype.toString = function() { return this.name; };
  StructType.prototype.getName = function() { return this.name; };

  StructType.prototype.equals = function(other) {
    if (this === other) {
      return true;
    }

    if (this.constructor !== other.constructor ||
        this.name !== other.name ||
        this.size !== other.size) {
      return false;
    }

    var thisFieldKeys = Object.keys(this.fields);
    var otherFieldKeys = Object.keys(other.fields);
    if (thisFieldKeys.length !== otherFieldKeys.length) {
      return false;
    }

    thisFieldKeys = thisFieldKeys.sort();
    otherFieldKeys = otherFieldKeys.sort();

    for (var i = 0; i < thisFieldKeys.length; ++i) {
      var thisFieldKey = thisFieldKeys[i];
      var otherFieldKey = otherFieldKeys[i];
      if (thisFieldKey !== otherFieldKey) {
        return false;
      }

      var thisFieldValue = this.fields[thisFieldKey];
      var otherFieldValue = other.fields[thisFieldKey];
      if (!thisFieldValue.equals(otherFieldValue)) {
        return false;
      }
    }

    return true;
  };

  StructType.prototype.addFields_ = function(fields) {
    for (var name in fields) {
      if (!fields.hasOwnProperty(name)) {
        continue;
      }

      var field = fields[name];
      this.addField_(name, field.type, field.offset);
    }
  };

  StructType.prototype.addField_ = function(name, type, offset) {
    assert(type instanceof Type, 'type is not instance of Type.');
    assert(offset >= 0, 'offset ' + offset + ' < 0');
    assert(offset + type.sizeof() <= this.size, 'offset ' + offset + ' > size');
    assert(!this.fields.hasOwnProperty(name),
           'field ' + name + ' already exists');
    this.fields[name] = new StructField(name, type, offset);
  };


  //// FunctionType ////////////////////////////////////////////////////////////
  function FunctionType(retType) {
    Type.call(this);
    this.retType = retType;
    this.argTypes = Array.prototype.slice.call(arguments, 1);

    // Validate the types.
    assert(retType instanceof Type, 'return type is not instance of Type');
    this.argTypes.forEach(function(argType, i) {
      assert(argType instanceof Type,
             'argument ' + i + ' is not instance of Type');
    });
  }

  FunctionType.prototype = new Type();
  FunctionType.prototype.constructor = FunctionType;

  FunctionType.prototype.sizeof = function() {
    return 4;
  };

  FunctionType.prototype.toString = function() {
    var s = '';
    s += this.retType.toString();
    s += ' (*)(';
    s += this.argTypes.map(function(x) { return x.toString(); }).join(', ');
    s += ')';
    return s;
  };

  FunctionType.prototype.equals = function(other) {
    if (this === other) {
      return true;
    }

    if (this.constructor !== other.constructor ||
        !this.retType.equals(other.retType)) {
      return false;
    }

    if (this.argTypes.length !== other.argTypes.length) {
      return false;
    }

    for (var i = 0; i < this.argTypes.length; ++i) {
      if (!this.argTypes[i].equals(other.argTypes[i])) {
        return false;
      }
    }

    return true;
  };


  //// PepperType //////////////////////////////////////////////////////////////
  function PepperType(name, jsPrototype) {
    Type.call(this);
    this.name = name;
    this.jsPrototype = jsPrototype;
  }

  PepperType.prototype = new Type();

  PepperType.prototype.sizeof = function() { return 20;  /* sizeof(PP_Var) */ };
  PepperType.prototype.toString = function() { return this.name; };
  PepperType.prototype.getName = function() { return this.name; };

  PepperType.prototype.equals = function(other) {
    if (this === other) {
      return true;
    }

    return this.constructor === other.constructor &&
           this.name === other.name &&
           this.jsPrototype === other.jsPrototype;
  };


  //// AliasType ///////////////////////////////////////////////////////////////
  function AliasType(name, type) {
    Type.call(this);
    this.name = name;
    this.type = type;
  }

  AliasType.prototype = new Type();
  AliasType.prototype.constructor = AliasType;
  AliasType.prototype.sizeof = function() { return this.type.sizeof(); };
  AliasType.prototype.toString = function() { return this.name; };
  AliasType.prototype.getName = function() { return this.name; };
  AliasType.prototype.isPrimitive = function() { return this.type.isPrimitive(); };
  AliasType.prototype.isPointer = function() { return this.type.isPointer(); };
  AliasType.prototype.isInt = function() { return this.type.isInt_; };
  AliasType.prototype.isSigned = function() { return this.type.isSigned_; };

  AliasType.prototype.equals = function(other) {
    return this.constructor === other.constructor &&
           this.name === other.name &&
           this.type.equals(other.type);
  };


  //// FunctionBuilder /////////////////////////////////////////////////////////
  function FunctionBuilder() {
    this.nameHash = {};
  }

  FunctionBuilder.prototype.makeFunction = function(name, overloads) {
    assert(!(name in this.nameHash), 'function with this name already exists.');

    var cfunc = new CFunction(name, overloads);
    var func = function(context) {
      var args = Array.prototype.slice.call(arguments, 1);
      return context.callFunction(cfunc, args);
    };

    this.nameHash[name] = func;
    return func;
  };

  FunctionBuilder.prototype.getNameHash = function() {
    return this.nameHash;
  };


  //// Functions ///////////////////////////////////////////////////////////////
  function CFunction(name, overloads) {
    this.name = name;
    if (overloads instanceof Array) {
      this.overloads = overloads;
    } else {
      this.overloads = [overloads];
    }

    // Quick sanity check.
    this.overloads.forEach(function(funcType) {
      assert(funcType instanceof FunctionType, 'expected FunctionType');
    });
  }


  //// Exports /////////////////////////////////////////////////////////////////
  return {
    makeModule: makeModule,
  };

});
