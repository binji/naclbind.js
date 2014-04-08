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

  StructField.prototype.set = function(context, struct_p, value) {
    var dst = context.func.add(struct_p, this.offset);
    return context.func.set(dst.cast(this.type.getPointerType()), value);
  };

  StructField.prototype.get = function(context, struct_p) {
    var ptr = context.func.add(struct_p, this.offset);
    return context.func.get(ptr.cast(this.type.getPointerType()));
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

  StructType.prototype.malloc = function(context) {
    return context.func.malloc(this.sizeof());
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

  //// Handle //////////////////////////////////////////////////////////////////
  var nextHandleId = 1;
  var handleIdHash = {};

  function Handle(context, type, id) {
    this.type = type;
    if (id !== undefined) {
      this.id = id;
    } else {
      this.id = nextHandleId++;
      handleIdHash[this.id] = this;
    }

    this.context = context;
    context.registerHandle(this);
  }

  Handle.prototype.toString = function() {
    return '[Handle ' + this.id + ' ' + this.type.toString() + ']';
  };

  Handle.prototype.cast = function(newType) {
    // TODO(binji): check validity of cast
    return new Handle(this.context, newType, this.id);
  };

  function getHandle(id) {
    return handleIdHash[id];
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

  var functions = {};

  function makeFunction(name, types) {
    assert(!(name in functions), '');
    var cfunc = new CFunction(name, types);
    functions[name] = cfunc;
  }


  function Context() {
    this.handles = [];
    this.messages = [];
    this.func = {};
    this.initializeFuncs_();
  }

  Context.prototype.initializeFuncs_ = function() {
    var that = this;

    function makeFunctionCaller(context, cfunc) {
      return function() {
        return context.call(cfunc, arguments);
      };
    }

    for (var name in functions) {
      var cfunc = functions[name];
      this.func[name] = makeFunctionCaller(that, cfunc);
    }
  };

  Context.prototype.registerHandle = function(handle) {
    assert(handle instanceof Handle, 'registerHandle: handle is not a Handle.');
    this.handles.push(handle);
  };

  Context.prototype.call = function(func, args) {
    assert(func instanceof CFunction, 'call: Expected func to be CFunction');

    var funcType = func.findOverload(args);
    assert(funcType !== null);

    var handle = new Handle(this, funcType.retType);
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

    this.messages.push(message);

    return handle;
  };

  Context.prototype.commit = function() {
    assert(arguments.length > 0, 'commit: Expected callback.');

    var callback = arguments[arguments.length - 1];
    assert(callback instanceof Function, 'commit: callback is not Function.');

    var args = Array.prototype.slice.call(arguments, 0, -1);

    var serializeHandle = function(handle) {
      assert(handle instanceof Handle, 'commit: handle is not a Handle.');
      return handle.id;
    };

    var handles = args.map(serializeHandle);
    var msg = {
      msgs: this.messages,
      handles: handles,
    };

    // Remove committed messages.
    this.messages = [];

    postMessage(msg, function(result) {
      function idToHandle(value, ix) {
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

  Context.prototype.commitPromise = function() {
    var that = this;
    var args = Array.prototype.slice.call(arguments);
    return new promise.PromisePlus(function(resolve, reject, resolveMany) {
      args.push(function() {
        resolveMany.apply(null, arguments);
      });
      that.commit.apply(that, args);
    });
  };

  // NaCl stuff...
  //////////////////////////////////////////////////////////////////////////////
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

  var makeContext = function() {
    return new Context();
  };

  // exported functions
  self.logTypes = logTypes;
  self.makeAliasType = makeAliasType;
  self.makeContext = makeContext;
  self.makeFunction = makeFunction;
  self.makeFunctionType = makeFunctionType;
  self.makePepperType = makePepperType;
  self.makePointerType = makePointerType;
  self.makePrimitiveType = makePrimitiveType;
  self.makeStructType = makeStructType;
  self.makeVoidType = makeVoidType;

}).call(nacl);
