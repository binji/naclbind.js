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

var nacl = {};
(function() {
  var self = this;

  function assert(cond, msg) {
    if (!cond) {
      throw new Error('Assertion failed' + (msg ? (': ' + msg) : '.'));
    }
  }

  self.makeModule = function(name, nmf, mimeType) {
    return new Module(name, nmf, mimeType);
  };

  // Module ////////////////////////////////////////////////////////////////////
  function Module(name, nmf, mimeType) {
    this.name = name;
    this.nmf = nmf;
    this.mimeType = mimeType;

    this.element = null;
    this.loaded = false;
    this.nextCallbackId = 1;
    this.idCallbackMap = [];
    this.queuedMessages = [];

    this.commands = [];

    this.types = new TypeList();
    this.handles = new HandleList();
    this.functions = new FunctionList();
    this.initDefaults_();

    this.createEmbed_();
  }

  Module.prototype.createEmbed_ = function() {
    var that = this;
    document.addEventListener('DOMContentLoaded', function() {
      that.element = document.createElement('embed');
      that.element.setAttribute('width', '0');
      that.element.setAttribute('height', '0');
      that.element.setAttribute('src', that.nmf);
      that.element.setAttribute('type', that.mimeType);

      that.element.addEventListener('load', that.onLoad_.bind(that), false);
      that.element.addEventListener('message', that.onMessage_.bind(that), false);
      that.element.addEventListener('error', that.onError_.bind(that), false);
      that.element.addEventListener('crash', that.onCrash_.bind(that), false);
      document.body.appendChild(that.element);
    });
  };

  Module.prototype.postMessage = function(msg, callback) {
    var id = this.nextCallbackId++;
    this.idCallbackMap[id] = callback;

    msg.id = id;
    if (this.loaded) {
      this.element.postMessage(msg);
    } else {
      this.queuedMessages.push(msg);
    }
    return id;
  };

  Module.prototype.makeContext = function() {
    return new Context(this);
  };

  Module.prototype.onLoad_ = function(event) {
    console.log('module loaded.');
    this.loaded = true;
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
      var callback = this.idCallbackMap[id];
      callback(msg);
      delete this.idCallbackMap[id];
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
    this.queuedMessages.forEach(function(msg) {
      that.element.postMessage(msg);
    });

    this.queuedMessages = null;
  };

  Module.prototype.callFunction = function(context, func, args) {
    assert(func instanceof CFunction, 'callFunction: Expected func to be CFunction');

    var funcType = this.findOverload(func.name, args, func.types);
    assert(funcType !== null);

    var handle = this.handles.makeHandle(context, funcType.retType);
    var message = {
      cmd: func.name,
      type: funcType.id,
      args: [],
      argIsHandle: [],
      ret: handle.id
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

    this.commands.push(message);

    return handle;
  };


  Module.prototype.commit = function(context) {
    assert(arguments.length > 1, 'commit: Expected callback.');

    var callback = arguments[arguments.length - 1];
    assert(callback instanceof Function, 'commit: callback is not Function.');

    // Slice off context (first element), and callback (last element).
    var args = Array.prototype.slice.call(arguments, 1, -1);

    var serializeHandle = function(handle) {
      assert(handle instanceof Handle, 'commit: handle is not a Handle.');
      return handle.id;
    };

    var handles = args.map(serializeHandle);
    var msg = {
      msgs: this.commands,  // TODO(binji): rename "msgs"
      handles: handles,
    };

    // Remove committed commands.
    this.commands = [];

    this.postMessage(msg, function(result) {
      function idToHandle(value, ix) {
        if (typeof(value) === 'number' && !args[ix].type.isPrimitive()) {
          return this.handles.getHandle(value);
        }
        return value;
      };

      // convert back from Ids to Handles; keep the primitive values the same.
      var handles = Array.prototype.map.call(result.values, idToHandle);
      callback.apply(null, handles);
    });
  };

  Module.prototype.commitPromise = function(context) {
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
    // TODO(binji): move these into their own object?
    this.void_ = this.types.makeVoidType(1);
    this.int8 = this.types.makePrimitiveType(2, 'int8', 1, true, true);
    this.uint8 = this.types.makePrimitiveType(3, 'uint8', 1, false, true);
    this.int16 = this.types.makePrimitiveType(4, 'int16', 2, true, true);
    this.uint16 = this.types.makePrimitiveType(5, 'uint16', 2, false, true);
    this.int32 = this.types.makePrimitiveType(6, 'int32', 4, true, true);
    this.uint32 = this.types.makePrimitiveType(7, 'uint32', 4, false, true);
    this.int64 = this.types.makePrimitiveType(8, 'int64', 8, true, true);
    this.uint64 = this.types.makePrimitiveType(9, 'uint64', 8, false, true);
    this.float32 = this.types.makePrimitiveType(10, 'float32', 4, false, false);
    this.float64 = this.types.makePrimitiveType(11, 'float64', 8, false, false);

    this.void_p = this.makePointerType(12, this.void_);
    this.int8_p = this.types.makePointerType(13, this.int8);
    this.uint8_p = this.makePointerType(14, this.uint8);
    this.int16_p = this.makePointerType(15, this.int16);
    this.uint16_p = this.makePointerType(16, this.uint16);
    this.int32_p = this.makePointerType(17, this.int32);
    this.uint32_p = this.makePointerType(18, this.uint32);
    this.int64_p = this.makePointerType(19, this.int64);
    this.uint64_p = this.makePointerType(20, this.uint64);
    this.float32_p = this.makePointerType(21, this.float32);
    this.float64_p = this.makePointerType(22, this.float64);
    this.void_pp = this.makePointerType(23, this.void_p);

    this.var_ = this.types.makePepperType(24, 'Var', undefined);
    this.arrayBuffer = this.types.makePepperType(25, 'ArrayBuffer', ArrayBuffer);
    this.array = this.types.makePepperType(26, 'Array', Array);
    this.dictionary = this.types.makePepperType(27, 'Dictionary', Object);

    var getTypes = [
      this.makeFunctionType(28, this.void_p, this.void_pp),
      this.makeFunctionType(29, this.int8, this.int8_p),
      this.makeFunctionType(30, this.uint8, this.uint8_p),
      this.makeFunctionType(31, this.int16, this.int16_p),
      this.makeFunctionType(32, this.uint16, this.uint16_p),
      this.makeFunctionType(33, this.int32, this.int32_p),
      this.makeFunctionType(34, this.uint32, this.uint32_p),
      this.makeFunctionType(35, this.int64, this.int64_p),
      this.makeFunctionType(36, this.uint64, this.uint64_p),
      this.makeFunctionType(37, this.float32, this.float32_p),
      this.makeFunctionType(38, this.float64, this.float64_p),
    ];

    var setTypes = [
      this.makeFunctionType(39, this.void_, this.void_pp, this.void_p),
      this.makeFunctionType(40, this.void_, this.int8_p, this.int8),
      this.makeFunctionType(41, this.void_, this.uint8_p, this.uint8),
      this.makeFunctionType(42, this.void_, this.int16_p, this.int16),
      this.makeFunctionType(43, this.void_, this.uint16_p, this.uint16),
      this.makeFunctionType(44, this.void_, this.int32_p, this.int32),
      this.makeFunctionType(45, this.void_, this.uint32_p, this.uint32),
      this.makeFunctionType(46, this.void_, this.int64_p, this.int64),
      this.makeFunctionType(47, this.void_, this.uint64_p, this.uint64),
      this.makeFunctionType(48, this.void_, this.float32_p, this.float32),
      this.makeFunctionType(49, this.void_, this.float64_p, this.float64),
    ];

    var freeType = this.makeFunctionType(50, this.void_, this.void_p);
    var mallocType = this.makeFunctionType(51, this.void_p, this.uint32);
    var memsetType = this.makeFunctionType(52, this.void_, this.void_p, this.int32, this.uint32);
    var memcpyType = this.makeFunctionType(53, this.void_, this.void_p, this.void_p, this.uint32);

    var addRefReleaseType = this.makeFunctionType(54, this.void_, this.var_);
    var arrayBufferCreateType = this.makeFunctionType(55, this.arrayBuffer, this.uint32);
    var arrayBufferMapType = this.makeFunctionType(56, this.void_p, this.arrayBuffer);
    var arrayBufferUnmapType = this.makeFunctionType(57, this.void_, this.arrayBuffer);

    var binopVoidpIntType = this.makeFunctionType(58, this.void_p, this.void_p, this.int32);
    var binopInt32Type = this.makeFunctionType(59, this.int32, this.int32, this.int32);
    var binopUint32Type = this.makeFunctionType(60, this.uint32, this.uint32, this.uint32);
    var binopInt64Type = this.makeFunctionType(61, this.int64, this.int64, this.int64);
    var binopUint64Type = this.makeFunctionType(62, this.uint64, this.uint64, this.uint64);
    var binopFloatType = this.makeFunctionType(63, this.float32, this.float32, this.float32);
    var binopDoubleType = this.makeFunctionType(64, this.float64, this.float64, this.float64);

    var addSubTypes = [
      binopVoidpIntType, binopInt32Type, binopUint32Type, binopInt64Type,
      binopUint64Type, binopFloatType, binopDoubleType,
    ];

    // builtin functions
    this.get = this.makeFunction('get', getTypes);
    this.set = this.makeFunction('set', setTypes);
    this.add = this.makeFunction('add', addSubTypes);
    this.sub = this.makeFunction('sub', addSubTypes);

    // stdlib
    this.free = this.makeFunction('free', freeType);
    this.malloc = this.makeFunction('malloc', mallocType);
    this.memcpy = this.makeFunction('memcpy', memcpyType);
    this.memset = this.makeFunction('memset', memsetType);

    // PPB_Var
    this.addRef = this.makeFunction('addRef', addRefReleaseType);
    this.release = this.makeFunction('release', addRefReleaseType);

    // PPB_VarArrayBuffer
    this.arrayBufferCreate = this.makeFunction('arrayBufferCreate', arrayBufferCreateType);
    this.arrayBufferMap = this.makeFunction('arrayBufferMap', arrayBufferMapType);
    this.arrayBufferUnmap = this.makeFunction('arrayBufferUnmap', arrayBufferUnmapType);
  };

  Module.prototype.makePointerType = function(id, baseType) {
    return this.types.makePointerType(id, baseType);
  };

  Module.prototype.makeStructType = function(id, size, name) {
    return this.types.makeStructType(id, size, name);
  };

  Module.prototype.makeFunctionType = function(id, retType) {
    return this.types.makeFunctionType.apply(this.types, arguments);
  };

  Module.prototype.makeFunction = function(name, types) {
    return this.functions.makeFunction(name, types);
  };

  // TODO(binji): Where should these go? On the context...?
  Module.prototype.setField = function(context, structField, struct_p, value) {
    var dst = this.add(context, struct_p, structField.offset);
    var pointerType = this.types.getPointerType(structField.type);
    if (structField.type.isPointer()) {
      return this.set(context, dst.cast(this.void_pp), value.cast(this.void_p));
    } else {
      return this.set(context, dst.cast(pointerType), value);
    }
  };

  Module.prototype.getField = function(context, structField, struct_p) {
    var ptr = this.add(context, struct_p, structField.offset);
    var pointerType = this.types.getPointerType(structField.type);
    if (structField.type.isPointer()) {
      return this.get(context, ptr.cast(this.void_pp)).cast(pointerType);
    } else {
      return this.get(context, ptr.cast(pointerType));
    }
  };

  Module.prototype.mallocType = function(context, type) {
    var pointerType = this.types.getPointerType(type);
    return this.malloc(context, type.sizeof()).cast(pointerType);
  };

  Module.prototype.findOverload = function(funcName, args, funcTypeList) {
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
          argType = this.float64;
        }
      } else if (arg instanceof ArrayBuffer) {
        argType = this.arrayBuffer;
      } else {
        // TODO(binji): handle other pepper types.
        // What kind of type is this?
        console.log('Unexpected type of arg "' + arg + '": ' + typeof(arg));
        return false;
      }

      if (argType !== funcArgType) {
        if (argType.isPointer() && funcArgType.isPointer() &&
            this.canCoercePointer_(argType, funcArgType)) {
          // OK
        } else if (argType.isPrimitive() && funcArgType.isPrimitive() &&
                   this.canCoercePrimitive_(argType, funcArgType)) {
          // OK
        } else {
          return false;
        }
      }
    }
    return true;
  };

  Module.prototype.getIntType_ = function(value) {
    if (value >= -128 && value <= 127) {
      return this.int8;
    } else if (value >= -32768 && value <= 32767) {
      return this.int16;
    } else if (value >= -2147483648 && value <= 2147483647) {
      return this.int32;
    // TODO(binji): JavaScript numbers only have 53-bits of precision, so
    // this is not correct. We need a 64-bit int type.
    } else if (value >= -9223372036854775808 &&
               value <=  9223372036854775807) {
      return this.int64;
    } else {
      assert(value > 0, 'getIntType_: Expected uint64. ' + value + ' <= 0.');
      return this.uint64;
    }
  };

  Module.prototype.canCoercePointer_ = function(fromType, toType) {
    assert(fromType.isPointer(),
           'canCoercePointer_: expected pointer, not ' + fromType);
    assert(toType.isPointer(),
           'canCoercePointer_: expected pointer, not ' + toType);
    // For now, we can only coerce pointers to void*. At some point, C++
    // inheritance could be supported as well.
    if (toType !== this.void_p) {
      //console.log('Can only coerce to void*, not ' + toType + '.');
      return false;
    }
    return true;
  };

  Module.prototype.canCoercePrimitive_ = function(fromType, toType) {
    assert(fromType.isPrimitive(),
           'canCoercePrimitive_: expected primitive, not ' + fromType);
    assert(toType.isPrimitive(),
           'canCoercePrimitive_: expected primitive, not ' + toType);

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
        if ((toType === this.float32 && fromType.sizeof() >= 4) ||
            (toType === this.float64 && fromType.sizeof() == 8)) {
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


  //// TypeList ////////////////////////////////////////////////////////////////
  function TypeList() {
    this.typeIdHash = {};
  }

  TypeList.prototype.registerType_ = function(id, newType) {
    assert(id !== 0, 'registerType_: id !== 0');
    assert(!(id in this.typeIdHash), 'registerType_: id ' + id + ' already exists');
    if (this.getTypeId(newType) !== 0) {
      throw new Error('Type ' + newType + ' already made with id ' + id);
    }
    newType.id = id;
    this.typeIdHash[id] = newType;
    return newType;
  };

  TypeList.prototype.makeAliasType = function(id, name, type) {
    return this.registerType_(id, new AliasType(name, type));
  };

  TypeList.prototype.makePepperType = function(id, name, type) {
    return this.registerType_(id, new PepperType(name, type));
  };

  TypeList.prototype.makePointerType = function(id, baseType) {
    return this.registerType_(id, new PointerType(baseType));
  };

  TypeList.prototype.makePrimitiveType = function(id, name, size, isSigned, isInt) {
    return this.registerType_(id, new PrimitiveType(name, size, isSigned, isInt));
  };

  TypeList.prototype.makeStructType = function(id, size, name) {
    return this.registerType_(id, new StructType(size, name));
  };

  TypeList.prototype.makeVoidType = function(id) {
    return this.registerType_(id, new VoidType(id));
  };

  TypeList.prototype.makeFunctionType = function(id, retType) {
    var args = Array.prototype.slice.call(arguments, 1);
    var constructor = Function.bind.apply(FunctionType, [null].concat(args));
    return this.registerType_(id, new constructor());
  };

  TypeList.prototype.getTypeId = function(type) {
    for (var id in this.typeIdHash) {
      if (this.typeIdHash[id].equals(type)) {
        return id;
      }
    }
    return 0;
  };

  TypeList.prototype.getPointerType = function(baseType) {
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
    return this.typeIdHash[id];
  };

  TypeList.prototype.log = function() {
    for (var id in this.typeIdHash) {
      if (this.typeIdHash.hasOwnProperty(id)) {
        console.log('id: ' + id + ' type: ' + this.typeIdHash[id]);
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
    assert(handle instanceof Handle, 'registerHandle: handle is not a Handle.');
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


  //// Handle //////////////////////////////////////////////////////////////////
  function Handle(handleList, context, type, id) {
    this.handleList = handleList;
    this.type = type;
    if (id !== undefined) {
      this.id = id;
    } else {
      this.id = handleList.registerHandle(this);
    }

    this.context = context;
    context.registerHandle(this);
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

  Type.prototype.isInt = function() { return false; }
  Type.prototype.isSigned = function() { return false; }

  //// VoidType ////////////////////////////////////////////////////////////////
  function VoidType() { Type.call(this); }
  VoidType.prototype = new Type();
  VoidType.prototype.constructor = VoidType;
  VoidType.prototype.sizeof = function() { return 0; }
  VoidType.prototype.toString = function() { return 'void'; }

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
  function StructType(size, name) {
    Type.call(this);
    this.size = size;
    this.name = name;
    this.fields = {};
  }

  StructType.prototype = new Type();
  StructType.prototype.constructor = StructType;

  StructType.prototype.sizeof = function() {
    return this.size;
  };

  StructType.prototype.toString = function() {
    return this.name;
  };

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

  StructType.prototype.addField = function(name, type, offset) {
    assert(offset >= 0, 'addField: offset ' + offset + ' < 0');
    assert(offset + type.sizeof() <= this.size,
           'addField: offset ' + offset + ' > size');
    assert(!this.fields.hasOwnProperty(name),
           'addField: field ' + name + ' already exists');
    this.fields[name] = new StructField(name, type, offset);
  };


  //// FunctionType ////////////////////////////////////////////////////////////
  function FunctionType(retType) {
    Type.call(this);
    this.retType = retType;
    this.argTypes = Array.prototype.slice.call(arguments, 1);

    // Validate the argument types.
    this.argTypes.forEach(function(argType, i) {
      if (!(argType instanceof Type)) {
        throw new Error('Argument #' + i +
                        ' of function is not an instance of Type.');
      }
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

  PepperType.prototype.sizeof = function() {
    return 20;  // PP_Var
  };

  PepperType.prototype.toString = function() {
    return this.name;
  };

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
  AliasType.prototype.sizeof = function() { return this.type.sizeof(); }
  AliasType.prototype.toString = function() { return this.name; }
  AliasType.prototype.isPrimitive = function() { return this.type.isPrimitive(); }
  AliasType.prototype.isPointer = function() { return this.type.isPointer(); }
  AliasType.prototype.isInt = function() { return this.type.isInt_; };
  AliasType.prototype.isSigned = function() { return this.type.isSigned_; };

  AliasType.prototype.equals = function(other) {
    return this.constructor === other.constructor &&
           this.name === other.name &&
           this.type.equals(other.type);
  };


  //// FunctionList ////////////////////////////////////////////////////////////
  function FunctionList() {
    this.functionNameHash = {};
  }

  FunctionList.prototype.makeFunction = function(name, types) {
    assert(!(name in this.functionNameHash), 'Function with this name already exists.');

    var cfunc = new CFunction(name, types);
    var func = function(context) {
      var args = Array.prototype.slice.call(arguments, 1);
      return context.callFunction(cfunc, args);
    };

    this.functionNameHash[name] = func;
    return func;
  };


  //// Functions ///////////////////////////////////////////////////////////////
  function CFunction(name, types) {
    this.name = name;
    if (types instanceof Array) {
      this.types = types;
    } else {
      this.types = [types];
    }

    // Quick sanity check.
    this.types.forEach(function(funcType) {
      assert(funcType instanceof FunctionType,
             'CFunction: expected FunctionType');
    });
  }

}).call(nacl);
