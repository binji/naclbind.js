// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

"use strict";

var nacl={};
(function() {
  var self = this;

  var moduleName = 'zlib-nacl';
  var nmf;
  var mimetype;
  if (false) {
    nmf = 'pnacl/Debug/zlib.nmf';
    mimetype = 'application/x-nacl';
  } else {
    nmf = 'pnacl/Release/zlib.nmf';
    mimetype = 'application/x-pnacl';
  }

  function assert(cond, msg) {
    if (!cond) {
      throw new Error('Assertion failed' + (msg ? (': ' + msg) : '.'));
    }
  }

  var typeIdHash = {};

  function makeType(id, typeConstructor, args) {
    assert(id !== 0, 'makeType: id !== 0');
    assert(!(id in typeIdHash), 'makeType: id ' + id + ' already exists');
    var newConstructor = typeConstructor.bind.apply(
        typeConstructor, [null].concat(args));
    var newType = new newConstructor();
    if (getTypeId(newType) !== 0) {
      throw new Error('Type ' + newType + ' already made with id ' + id);
    }
    newType.id = id;
    typeIdHash[id] = newType;
    return newType;
  }

  function makeMakeTypeFunction(typeConstructor) {
    return function(id) {
      return makeType(id, typeConstructor,
                      Array.prototype.slice.call(arguments, 1));
    }
  }

  function getTypeId(type) {
    for (var id in typeIdHash) {
      if (typeIdHash[id].equals(type)) {
        return id;
      }
    }
    return 0;
  }

  var logTypes = function() {
    for (var id in typeIdHash) {
      if (typeIdHash.hasOwnProperty(id)) {
        console.log('id: ' + id + ' type: ' + typeIdHash[id]);
      }
    }
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

  Type.prototype.getPointerType = function() {
    if (this.pointerType) {
      return this.pointerType;
    }

    var pointerType = new PointerType(this);
    var id = getTypeId(pointerType);
    if (id === 0) {
      throw new Error('Pointer type "' + pointerType + '" not defined.');
    }

    // Get the correct one.
    this.pointerType = pointerType = typeIdHash[id];
    return pointerType;
  };

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

  StructField.prototype.set = function(struct_p, value) {
    var dst = add(struct_p, this.offset);
    return set(dst.cast(this.type.getPointerType()), value);
  };

  StructField.prototype.get = function(struct_p) {
    var ptr = add(struct_p, this.offset);
    return get(ptr.cast(this.type.getPointerType()));
  };

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

  StructType.prototype.malloc = function() {
    return malloc(this.sizeof());
  };

  //// FunctionType ////////////////////////////////////////////////////////////
  function FunctionType(retType) {
    Type.call(this);
    this.retType = retType;
    this.argTypes = Array.prototype.slice.call(arguments, 1);
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
    return 20;  // pp::Var
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

  // Handle
  var nextHandleId = 1;
  var handleIdHash = {};

  function Handle(type, id) {
    this.type = type;
    if (id !== undefined) {
      this.id = id;
    } else {
      this.id = nextHandleId++;
      handleIdHash[this.id] = this;
    }
  }

  Handle.prototype.toString = function() {
    return '[Handle ' + this.id + ' ' + this.type.toString() + ']';
  };

  Handle.prototype.cast = function(newType) {
    // TODO(binji): check validity of cast
    return new Handle(newType, this.id);
  };

  function deleteHandle(id) {
    delete handleIdHash[id];
  }

  function getHandle(id) {
    return handleIdHash[id];
  }

  function collectAllHandlesExcept(used) {
    // Build a new hash with the used elements.
    var newHash = {};
    used.forEach(function(id) {
      var handle = handleIdHash[id];
      newHash[id] = handle;
    });

    handleIdHash = newHash;
  }

  // Functions
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

  CFunction.prototype.findOverload = function(args) {
    for (var i = 0; i < this.types.length; ++i) {
      if (CFunction.overloadMatches(this.types[i], args)) {
        return this.types[i];
      }
    }

    // Display helpful warning.
    var msg;
    msg = 'No overload found for call "' + this.name + '(';
    msg += Array.prototype.join.call(args, ', ');
    msg += ')".\n';
    msg += "Possibilities:\n";
    for (var i = 0; i < this.types.length; ++i) {
      msg += this.types[i].toString() + "\n";
    }
    console.log(msg);

    return null;
  };

  CFunction.getIntType = function(value) {
    if (value >= -128 && value <= 127) {
      return int8;
    } else if (value >= -32768 && value <= 32767) {
      return int16;
    } else if (value >= -2147483648 && value <= 2147483647) {
      return int32;
    // TODO(binji): JavaScript numbers only have 53-bits of precision, so
    // this is not correct. We need a 64-bit int type.
    } else if (value >= -9223372036854775808 &&
               value <=  9223372036854775807) {
      return int64;
    } else {
      assert(value > 0, 'getIntType: Expected uint64. ' + value + ' <= 0.');
      return uint64;
    }
  };

  CFunction.canCoercePointer = function(fromType, toType) {
    assert(fromType.isPointer(),
           'canCoercePointer: expected pointer, not ' + fromType);
    assert(toType.isPointer(),
           'canCoercePointer: expected pointer, not ' + toType);
    // For now, we can only coerce pointers to void*. At some point, C++
    // inheritance could be supported as well.
    if (toType !== void_p) {
      //console.log('Can only coerce to void*, not ' + toType + '.');
      return false;
    }
    return true;
  };

  CFunction.canCoercePrimitive = function(fromType, toType) {
    assert(fromType.isPrimitive(),
           'canCoercePrimitive: expected primitive, not ' + fromType);
    assert(toType.isPrimitive(),
           'canCoercePrimitive: expected primitive, not ' + toType);

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
        if ((toType === float32 && fromType.sizeof() >= 4) ||
            (toType === float64 && fromType.sizeof() == 8)) {
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

  CFunction.overloadMatches = function(funcType, args) {
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
          argType = CFunction.getIntType(arg);
        } else {
          // Float.
          argType = float64;
        }
      } else if (arg instanceof ArrayBuffer) {
        argType = arrayBuffer;
      } else {
        // What kind of type is this?
        console.log('Unexpected type of arg "' + arg + '": ' + typeof(arg));
        return false;
      }

      if (argType !== funcArgType) {
        if (argType.isPointer() && funcArgType.isPointer() &&
            CFunction.canCoercePointer(argType, funcArgType)) {
          // OK
        } else if (argType.isPrimitive() && funcArgType.isPrimitive() &&
                   CFunction.canCoercePrimitive(argType, funcArgType)) {
          // OK
        } else {
          return false;
        }
      }
    }
    return true;
  };

  function makeFunction(name, types) {
    var cfunc = new CFunction(name, types);
    return function() {
      return call(cfunc, arguments);
    };
  }


  // TODO don't use global...?
  var messages = [];

  function call(func, args) {
    assert(func instanceof CFunction, 'call: Expected func to be CFunction');
    // Find the correct overload.
    var funcType = func.findOverload(args);
    assert(funcType !== null);

    var handle = new Handle(funcType.retType);
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

    messages.push(message);

    return handle;
  };

  function commit() {
    assert(arguments.length > 0, 'commit: Expected callback.');

    var callback = arguments[arguments.length - 1];
    assert(callback instanceof Function, 'commit: callback is not Function.');

    var args = Array.prototype.slice.call(arguments, 0, -1);

    var serializeHandle = function(handle) {
      assert(handle instanceof Handle, 'commit: handle is not a Handle.');
      return handle.id;
    };

    var handles = Array.prototype.map.call(args, serializeHandle);
    var msg = {
      msgs: messages,
      handles: handles,
    };

    // Remove committed messages.
    messages = [];

    postMessage(msg, function(result) {
      // Remove unused handles.
      // TODO Collect non-pointer handles.
      //collectAllHandlesExcept(handleIds);

      var idToHandle = function(value, ix) {
        if (typeof(value) === 'number' && !args[ix].type.isPrimitive()) {
          return getHandle(value);
        }
        return value;
      };

      // convert back from Ids to Handles; keep the primitive values the same.
      var handles = Array.prototype.map.call(result.values, idToHandle);
      callback.apply(null, handles);
    });
  };

  function ArgumentsWrapper(args) {
    this.args = args;
  }

  function wrapArguments(args) {
    return new ArgumentsWrapper(args);
  }

  function wrapPromise(p) {
    if (p instanceof NaClPromise) {
      return p;
    }
    return new NaClPromise(p);
  }

  function NaClPromise(f) {
    if (f instanceof Promise) {
      this.promise = f;
    } else {
      this.promise = new Promise(f);
    }
  }
  NaClPromise.prototype = Object.create(Promise.prototype);
  NaClPromise.prototype.constructor = NaClPromise;

  NaClPromise.resolve = function(x) {
    return new NaClPromise(function(resolve) { resolve(x); });
  };

  NaClPromise.reject = function(x) {
    return new NaClPromise(function(resolve, reject) { reject(x); });
  };

  NaClPromise.prototype.catch = function(reject) {
    return wrapPromise(this.promise.catch(reject));
  };

  NaClPromise.prototype.then = function(resolve, reject) {
    return wrapPromise(this.promise.then(function(value) {
      if (value instanceof ArgumentsWrapper) {
        return resolve.apply(null, value.args);
      } else {
        return resolve.apply(null, args);
      }
    }, reject));
  };

  function return1(x) { return function() { return x; } }
  function reject1(x) { return function() { return NaClPromise.reject(x); } }

  NaClPromise.prototype.finally = function(f) {
    return this.then(function(x) {
      return resolve().then(f).then(return1(x));
    }, function(x) {
      return resolve().then(f).then(reject1(x));
    });
  };

  NaClPromise.prototype.if = function(cond, trueBlock, falseBlock) {
    return this.then(function(prev) {
      return resolve(prev).then(cond).then(function(condResult) {
        if (condResult) {
          return resolve(prev).then(trueBlock);
        } else {
          if (falseBlock) {
            return resolve(prev).then(falseBlock);
          } else {
            return undefined;
          }
        }
      });
    });
  };

  NaClPromise.prototype.while = function(cond, block) {
    return this.then(function loop() {
      var args = arguments;
      return resolveMany(args).then(cond).then(function(condResult) {
        if (condResult) {
          return resolveMany(args).then(block).then(function() {
            return loop.apply(null, arguments);
          });
        } else {
          return resolveMany(args);
        }
      });
    });
  };

  function commitPromise() {
    var args = Array.prototype.slice.call(arguments);
    return new NaClPromise(function(resolve) {
      args.push(function() {
        resolve(wrapArguments(arguments));
      });
      nacl.commit.apply(null, args);
    });
  }

  function resolve(value) {
    return NaClPromise.resolve(value);
  }

  function resolveMany() {
    return NaClPromise.resolve(wrapArguments(arguments));
  }

  function reject(value) {
    return NaClPromise.reject(value);
  }


  // NaCl stuff...
  var nextCallbackId = 1;
  var idCallbackMap = [];
  var moduleEl;
  var moduleLoaded = false;
  var queuedMessages = [];

  function postMessage(msg, callback) {
    var id = nextCallbackId++;
    idCallbackMap[id] = callback;

    msg.id = id;
    if (moduleLoaded) {
      moduleEl.postMessage(msg);
    } else {
      queuedMessages.push(msg);
    }
    return id;
  }

  function postQueuedMessages() {
    queuedMessages.forEach(function(msg) {
      moduleEl.postMessage(msg);
    });

    queuedMessages = [];
  }

  function onModuleLoad(event) {
    console.log('module loaded.');
    moduleLoaded = true;
    postQueuedMessages();
  }

  function onModuleMessage(event) {
    var msg = event.data;
    if (typeof(msg) !== 'object') {
      var msg = moduleName + ': unexpected value from module: ' +
          JSON.stringify(msg);
      throw new Error(msg);
    }

    if (msg.msg) {
      console.log(msg.msg);
      return;
    }

    var id = msg.id;
    if (id !== 0) {
      var callback = idCallbackMap[id];
      callback(msg);
      delete idCallbackMap[id];
    }
  }

  function onModuleError(event) {
    var msg = moduleName + ': error loading NaCl module: ' + moduleEl.lastError;
    throw new Error(msg);
  }

  function onModuleCrash(event) {
    var msg = moduleName + ': NaCl module crashed: ' + moduleEl.exitStatus;
    throw new Error(msg);
  }

  // Create NaCl module.
  document.addEventListener('DOMContentLoaded', function() {
    var listenerEl = document.createElement('div');
    moduleEl = document.createElement('embed');
    moduleEl.setAttribute('width', '0');
    moduleEl.setAttribute('height', '0');
    moduleEl.setAttribute('src', nmf);
    moduleEl.setAttribute('type', mimetype);

    listenerEl.addEventListener('load', onModuleLoad, true);
    listenerEl.addEventListener('message', onModuleMessage, true);
    listenerEl.addEventListener('error', onModuleError, true);
    listenerEl.addEventListener('crash', onModuleCrash, true);

    listenerEl.appendChild(moduleEl);
    document.body.appendChild(listenerEl);
  });

  var makeAliasType = makeMakeTypeFunction(AliasType);
  var makePepperType = makeMakeTypeFunction(PepperType);
  var makePointerType = makeMakeTypeFunction(PointerType);
  var makePrimitiveType = makeMakeTypeFunction(PrimitiveType);
  var makeStructType = makeMakeTypeFunction(StructType);
  var makeVoidType = makeMakeTypeFunction(VoidType);
  var makeFunctionType = makeMakeTypeFunction(FunctionType);

  // Built-in types. These should be auto-generated so they match NaCl, but for
  // now just generate them.
  // TODO intern new types?
  var void_ = makeVoidType(1);
  var int8 = makePrimitiveType(2, 'int8', 1, true, true);
  var uint8 = makePrimitiveType(3, 'uint8', 1, false, true);
  var int16 = makePrimitiveType(4, 'int16', 2, true, true);
  var uint16 = makePrimitiveType(5, 'uint16', 2, false, true);
  var int32 = makePrimitiveType(6, 'int32', 4, true, true);
  var uint32 = makePrimitiveType(7, 'uint32', 4, false, true);
  var int64 = makePrimitiveType(8, 'int64', 8, true, true);
  var uint64 = makePrimitiveType(9, 'uint64', 8, false, true);
  var float32 = makePrimitiveType(10, 'float32', 4, false, false);
  var float64 = makePrimitiveType(11, 'float64', 8, false, false);
  var size_t = makeAliasType(12, 'size_t', uint32);

  var void_p = makePointerType(13, void_);
  var uint8_p = makePointerType(14, uint8);
  var uint8_pp = makePointerType(15, uint8_p);
  var uint32_p = makePointerType(16, uint32);

  var var_ = makePepperType(17, 'Var', undefined);
  var arrayBuffer = makePepperType(18, 'ArrayBuffer', ArrayBuffer);
  var array = makePepperType(19, 'Array', Array);
  var dictionary = makePepperType(20, 'Dictionary', Object);
  var addRefReleaseType = makeFunctionType(21, void_, var_);
  var freeType = makeFunctionType(22, void_, void_p);
  var mallocType = makeFunctionType(23, void_p, size_t);
  var memsetType = makeFunctionType(24, void_, void_p, int32, size_t);
  var memcpyType = makeFunctionType(25, void_, void_p, void_p, size_t);
  var addVoidpInt32Type = makeFunctionType(26, void_p, void_p, int32);
  var setUint8pType = makeFunctionType(27, void_, uint8_pp, uint8_p);
  var setUint32Type = makeFunctionType(28, void_, uint32_p, uint32);
  var getUint8pType = makeFunctionType(29, uint8_p, uint8_pp);
  var getUint32Type = makeFunctionType(30, uint32, uint32_p);
  var subInt32Type = makeFunctionType(31, int32, int32, int32);
  var subUint32Type = makeFunctionType(32, uint32, uint32, uint32);

  var arrayBufferCreateType = makeFunctionType(33, arrayBuffer, uint32);
  var arrayBufferMapType = makeFunctionType(34, void_p, arrayBuffer);
  var arrayBufferUnmapType = makeFunctionType(35, void_, arrayBuffer);

  // Built-in functions.
  var add = makeFunction('add', addVoidpInt32Type);
  var addRef = makeFunction('addRef', addRefReleaseType);
  var arrayBufferCreate = makeFunction('arrayBufferCreate', arrayBufferCreateType);
  var arrayBufferMap = makeFunction('arrayBufferMap', arrayBufferMapType);
  var free = makeFunction('free', freeType);
  var get = makeFunction('get', [getUint8pType, getUint32Type]);
  var malloc = makeFunction('malloc', mallocType);
  var memcpy = makeFunction('memcpy', memcpyType);
  var memset = makeFunction('memset', memsetType);
  var release = makeFunction('release', addRefReleaseType);
  var set = makeFunction('set', [setUint8pType, setUint32Type]);
  var sub = makeFunction('sub', [subInt32Type, subUint32Type]);

  self.userTypeId = 36;


  // exported Types
  self.array = array;
  self.arrayBuffer = arrayBuffer;
  self.dictionary = dictionary;
  self.float32 = float32;
  self.float64 = float64;
  self.int16 = int16;
  self.int32 = int32;
  self.int64 = int64;
  self.int8 = int8;
  self.size_t = size_t;
  self.uint16 = uint16;
  self.uint32_p = uint32_p;
  self.uint32 = uint32;
  self.uint64 = uint64;
  self.uint8_p = uint8_p;
  self.uint8_pp = uint8_pp;
  self.uint8 = uint8;
  self.void_p = void_p;
  self.void = void_;

  // exported CFunctions
  self.add = add;
  self.addRef = addRef;
  self.arrayBufferCreate = arrayBufferCreate;
  self.arrayBufferMap = arrayBufferMap;
  self.free = free;
  self.malloc = malloc;
  self.memcpy = memcpy;
  self.memset = memset;
  self.release = release;
  self.sub = sub;

  // exported functions
  self.commit = commit;
  self.commitPromise = commitPromise;
  self.makeFunction = makeFunction;
  self.makeFunctionType = makeFunctionType;
  self.makePointerType = makePointerType;
  self.makeStructType = makeStructType;
  self.logTypes = logTypes;
  self.reject = reject;
  self.resolve = resolve;
  self.resolveMany = resolveMany;

}).call(nacl);
