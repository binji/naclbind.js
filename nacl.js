// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

"use strict";

var nacl={};
(function() {
  var self = this;

  var moduleName = 'zlib-nacl';
  //var nmf = 'pnacl/Debug/zlib.nmf';
  //var mimetype = 'application/x-nacl';
  var nmf = 'pnacl/Release/zlib.nmf';
  var mimetype = 'application/x-pnacl';

  function assert(cond, msg) {
    if (!cond) {
      throw new Error('Assertion failed' + (msg ? (': ' + msg) : '.'));
    }
  }

  var typeIdHash = {};

  function makeType(id, newType) {
    assert(id !== 0);
    assert(!(id in typeIdHash));
    if (getTypeId(newType) !== 0) {
      throw new Error('Type ' + newType + ' already made with id ' + id);
    }
    typeIdHash[id] = newType;
  }

  function getTypeId(type) {
    for (var id in typeIdHash) {
      if (typeIdHash[id].equals(newtype)) {
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
  function Type() {}

  Type.prototype.equals = function(other) {
    return false;
  };

  // Convenience functions.
  Type.prototype.isPrimitive = function() {
    return this instanceof PrimitiveType;
  };

  Type.prototype.isPointer = function() {
    return this instanceof PointerType;
  };

  //// VoidType ////////////////////////////////////////////////////////////////
  function VoidType() { Type(); }
  VoidType.prototype = new Type();
  VoidType.prototype.sizeof = function() { return 0; }
  VoidType.prototype.toString = function() { return 'void'; }

  //// PrimitiveType ///////////////////////////////////////////////////////////
  function PrimitiveType(name, size, isSigned, isInt) {
    Type.call(this);
    this.name = name;
    this.size = size;
    this.isSigned = isSigned;
    this.isInt = isInt;
  }

  PrimitiveType.prototype = new Type();

  PrimitiveType.prototype.sizeof = function() {
    return this.size;
  };

  PrimitiveType.prototype.toString = function() {
    return this.name;
  };

  //// PointerType /////////////////////////////////////////////////////////////
  function PointerType(baseType) {
    Type.call(this);
    this.baseType = baseType;
  }

  PointerType.prototype = new Type();

  PointerType.prototype.sizeof = function() {
    return 4;  // NaCl pointers are always 32-bit.
  };

  PointerType.prototype.toString = function() {
    return this.baseType.toString() + '*';
  };

  //// StructField /////////////////////////////////////////////////////////////
  function StructField(name, type, offset) {
    this.name = name;
    this.type = type;
    this.offset = offset;
  }

  StructField.prototype.set = function(struct_p, value) {
    var dst = self.call(self.add, struct_p, this.offset);
    return self.call(self.set, dst, value);
  };

  StructField.prototype.get = function (struct_p) {
  };

  //// StructType //////////////////////////////////////////////////////////////
  function StructType(size, name) {
    Type.call(this);
    this.size = size;
    this.name = name;
    this.fields = {};
  }

  function makeStructType(id, size, name) {
    var newType = new StructType(size, name);
    makeType(id, newType);
    return newType;
  };

  StructType.prototype = new Type();
  StructType.prototype.constructor = StructType;

  StructType.prototype.sizeof = function() {
    return this.size;
  };

  StructType.prototype.toString = function() {
    return this.name;
  };

  StructType.prototype.addField = function(name, type, offset) {
    assert(offset >= 0);
    assert(offset + type.sizeof() <= this.size);
    assert(!this.fields.hasOwnProperty(name));
    this.fields[name] = new StructField(name, type, offset);
  };

  StructType.prototype.malloc = function() {
    return self.call(malloc, this.sizeof());
  };

  //// FunctionType ////////////////////////////////////////////////////////////
  function FunctionType(retType) {
    Type.call(this);
    this.retType = retType;
    this.argTypes = Array.prototype.slice.call(arguments, 1);
  }

  FunctionType.prototype = new Type();

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

  //// PepperType //////////////////////////////////////////////////////////////
  function PepperType(name, jsPrototype) {
    Type();
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
      assert(funcType instanceof FunctionType);
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
    msg += args.join(', ');
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
      assert(value > 0);
      return uint64;
    }
  };

  CFunction.canCoercePointer = function(fromType, toType) {
    assert(fromType instanceof PointerType);
    assert(toType instanceof PointerType);
    // For now, we can only coerce pointers to void*. At some point, C++
    // inheritance could be supported as well.
    if (toType !== void_p) {
      console.log('Can only coerce to void*, not ' + toType + '.');
      return false;
    }
    return true;
  };

  CFunction.canCoercePrimitive = function(fromType, toType) {
    assert(fromType instanceof PrimitiveType);
    assert(toType instanceof PrimitiveType);

    if (fromType.isInt === toType.isInt) {
      if (fromType.isInt) {
        // Both ints.
        if (fromType.sizeof() > toType.sizeof()) {
          console.log('Argument type is too large: ' + fromType + ' > ' +
                      toType + '.');
          return false;
        } else if (fromType.sizeof() === toType.sizeof() &&
                   fromType.isSigned !== toType.isSigned) {
          console.log('Signed/unsigned mismatch: ' + fromType + ', ' +
                      toType + '.');
          return false;
        }
      } else {
        // Both floats.
        if (fromType.sizeof() > toType.sizeof()) {
          console.log('Argument type is too large: ' + fromType + ' > ' +
                      toType + '.');
          return false;
        }
      }
    } else {
      // One int, one float.
      if (fromType.isInt) {
        // From int to float.
        if ((toType === float32 && fromType.sizeof() >= 4) ||
            (toType === float64 && fromType.sizeof() == 8)) {
          console.log('Argument type is too large: ' + fromType + ' > ' +
                      toType + '.');
          return false;
        }
      } else {
        // From float to int.
        console.log('Implicit cast from float to int: ' + fromType + ' => ' +
                    toType + '.');
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


  // TODO don't use global...?
  var messages = [];

  function call(func) {
    assert(func instanceof CFunction);
    var args = Array.prototype.slice.call(arguments, 1);
    // Find the correct overload.
    var funcType = func.findOverload(args);
    assert(funcType !== null);

    var handle = new Handle(funcType.retType);
    var message = {
      cmd: func.name,
      type: funcType.id,
      args: [],
      ret: handle.id
    };
    for (var i = 0; i < funcType.argTypes.length; ++i) {
      // TODO check argument against argType.
      var arg = arguments[i + 1];
      assert(arg !== undefined);

      var value;
      if (arg instanceof Handle) {
        value = arg.id;
      } else {
        value = arg;
      }
      message.args.push(value);
    }

    messages.push(message);

    return handle;
  };

  function commit() {
    assert(arguments.length > 0);

    var callback = arguments[arguments.length - 1];
    assert(callback instanceof Function);

    var args = Array.prototype.slice.call(arguments, 0, -1);

    var serializeHandle = function(handle) {
      assert(handle instanceof Handle);
      return [handle.id, handle.type.id];
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

  // Built-in types. These should be auto-generated so they match NaCl, but for
  // now just generate them.
  // TODO intern new types?
  var void = makeType(1, new VoidType());
  var int8 = makeType(2, new PrimitiveType('int8', 1, true, true));
  var uint8 = makeType(3, new PrimitiveType('uint8', 1, false, true));
  var int16 = makeType(4, new PrimitiveType('int16', 2, true, true));
  var uint16 = makeType(5, new PrimitiveType('uint16', 2, false, true));
  var int32 = makeType(6, new PrimitiveType('int32', 4, true, true));
  var uint32 = makeType(7, new PrimitiveType('uint32', 4, false, true));
  var int64 = makeType(8, new PrimitiveType('int64', 8, true, true));
  var uint64 = makeType(9, new PrimitiveType('uint64', 8, false, true));
  var float32 = makeType(10, new PrimitiveType('float32', 4, false, false));
  var float64 = makeType(11, new PrimitiveType('float64', 8, false, false));
  // TODO alias size_t => uint32?
  var size_t = makeType(12, new PrimitiveType('size_t', 4, false, true));

  var void_p = makeType(13, new PointerType(void));
  var uint8_p = makeType(14, new PointerType(uint8));
  var uint32_p = makeType(15, new PointerType(uint32));

  var arrayBuffer = makeType(16, new PepperType('ArrayBuffer', ArrayBuffer));
  var array = makeType(17, new PepperType('Array', Array));
  var dictionary = makeType(18, new PepperType('Dictionary', Object));
  var mallocType = makeType(19, new FunctionType(void_p, size_t));
  var memsetType = makeType(20, new FunctionType(void, void_p, int32, size_t));
  var memcpyType = makeType(21, new FunctionType(void, void_p, void_p, size_t));
  var mapArrayBufferType = makeType(22, new FunctionType(void_p, arrayBuffer));
  var addVoidpInt32Type = makeType(23, new FunctionType(void_p, void_p, int32));

  // Built-in functions.
  var malloc = new CFunction('malloc', mallocType);
  var memset = new CFunction('memset', memsetType);
  var memcpy = new CFunction('memcpy', memcpyType);
  var mapArrayBuffer = new CFunction('mapArrayBuffer', mapArrayBufferType);
  var add = new CFunction('add', addVoidpInt32Type);
  //var set = new CFunction('set', []);

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

  // exported Types
  self.void = void;
  self.void_p = void_p;
  self.int8 = int8;
  self.uint8 = uint8;
  self.int16 = int16;
  self.uint16 = uint16;
  self.int32 = int32;
  self.uint32 = uint32;
  self.int64 = int64;
  self.uint64 = uint64;
  self.float32 = float32;
  self.float64 = float64;
  self.size_t = size_t;
  self.uint8_p = uint8_p;
  self.uint8_pp = uint8_pp;
  self.uint32_p = uint32_p;
  self.arrayBuffer = arrayBuffer;
  self.array = array;
  self.dictionary = dictionary;

  // exported CFunctions
  self.malloc = malloc;
  self.memset = memset;
  self.memcpy = memcpy;
  self.mapArrayBuffer = mapArrayBuffer;
  self.add = add;

  // exported functions
  self.makeStructType = makeStructType;
  self.call;
  self.commit;

}).call(nacl);
