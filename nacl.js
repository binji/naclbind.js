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

  // Type IDs. These should be auto-generated so they match NaCl, but for now
  // just generate them.
  var nextTypeId = 1;
  var typeIdHash = {};

  self.Type = function() {
    this.id = nextTypeId++;
    typeIdHash[this.id] = this;
  };

  self.PrimitiveType = function(size, name) {
    self.Type();
    this.size = size;
    this.name = name;
  };

  self.PrimitiveType.prototype = new self.Type();

  self.PrimitiveType.prototype.sizeof = function() {
    return this.size;
  };

  self.PrimitiveType.prototype.toString = function() {
    return this.name;
  };

  self.PointerType = function(baseType) {
    self.Type();
    this.baseType = baseType;
  }

  self.PointerType.prototype = new self.Type();

  self.PointerType.prototype.sizeof = function() {
    return 4;  // NaCl pointers are always 32-bit.
  };

  self.PointerType.prototype.toString = function() {
    return this.baseType.toString() + '*';
  };

  self.StructField = function(name, type, offset) {
    this.name = name;
    this.type = type;
    this.offset = offset;
  };

  self.StructField.prototype.set = function(struct_p, value) {
    var dst = self.add(struct_p, this.offset);
    var src = self.addr(value);
    return self.call(self.memcpy, dst, src, this.type.sizeof());
  };

  self.StructType = function(size, name) {
    self.Type();
    this.size = size;
    this.name = name;
    this.fields = {};
  };

  self.StructType.prototype = new self.Type();

  self.StructType.prototype.sizeof = function() {
    return this.size;
  };

  self.StructType.prototype.toString = function() {
    return this.name;
  };

  self.StructType.prototype.addField = function(name, type, offset) {
    assert(offset >= 0);
    assert(offset + type.sizeof() <= this.size);
    assert(!this.fields.hasOwnProperty(name));
    this.fields[name] = new self.StructField(name, type, offset);
  };

  self.StructType.prototype.malloc = function() {
    var handle = self.call(self.malloc, this.sizeof());
    handle.type = this;
    return handle;
  };

  // Handle
  var nextHandleId = 1;
  var handleIdHash = {};

  self.Handle = function(type) {
    this.id = nextHandleId++;
    this.type = type;
    handleIdHash[this.id] = this;
  };

  self.Handle.prototype.toString = function() {
    return '[Handle ' + this.id + ' ' + this.type.toString() + ']';
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
  self.FunctionType = function(retType) {
    self.Type();
    this.retType = retType;
    this.argTypes = Array.prototype.slice.call(arguments, 1);
  };

  self.FunctionType.prototype = new self.Type();

  self.FunctionType.prototype.sizeof = function() {
    return 4;
  };

  self.FunctionType.prototype.toString = function() {
    var s = '';
    s += self.retType.toString();
    s += ' (*)(';
    s += self.argTypes.map(function(x) { return x.toString(); }).join(', ');
    s += ')';
    return s;
  };

  // Functions
  self.CFunction = function(name, type) {
    this.name = name;
    this.type = type;
  };


  // TODO don't use global...?
  var messages = [];

  self.call = function(func) {
    assert(func.type instanceof self.FunctionType);
    assert(func.type.argTypes.length === arguments.length - 1);

    var handle = new self.Handle(func.type.retType);
    var message = {cmd: func.name, args: [], ret: handle.id};
    for (var i = 0; i < func.type.argTypes.length; ++i) {
      // TODO check argument against argType.
      var arg = arguments[i + 1];
      assert(arg !== undefined);

      var value;
      if (arg instanceof self.Handle) {
        value = arg.id;
      } else {
        value = arg;
      }
      message.args.push(value);
    }

    messages.push(message);

    return handle;
  };

  self.commit = function() {
    assert(arguments.length > 0);

    var callback = arguments[arguments.length - 1];
    assert(callback instanceof Function);

    var args = Array.prototype.slice.call(arguments, 0, -1);

    var getId = function(handle) {
      assert(handle instanceof self.Handle);
      return handle.id;
    };

    var handleIds = Array.prototype.map.call(args, getId);
    var msg = {
      msgs: messages,
      handles: handleIds,
    };

    // Remove committed messages.
    messages = [];

    postMessage(msg, function() {
      // Remove unused handles.
      // TODO Collect non-pointer handles.
      collectAllHandlesExcept(handleIds);

      deleteCallback(callbackId);

      // convert back from Ids to Handles.
      var handles = Array.prototype.map.call(handleIds, getHandle);
      callback.apply(null, handles);
    });
  };

  // Built-in types. These should be auto-generated so they match NaCl, but for
  // now just generate them.
  // TODO intern new types?
  self.void = new self.PrimitiveType(0, 'void');
  self.void_p = new self.PointerType(self.void);
  self.int8 = new self.PrimitiveType(1, 'int8');
  self.uint8 = new self.PrimitiveType(1, 'uint8');
  self.int16 = new self.PrimitiveType(2, 'int16');
  self.uint16 = new self.PrimitiveType(2, 'uint16');
  self.int32 = new self.PrimitiveType(4, 'int32');
  self.uint32 = new self.PrimitiveType(4, 'uint32');
  self.int64 = new self.PrimitiveType(8, 'int64');
  self.uint64 = new self.PrimitiveType(8, 'uint64');
  self.float32 = new self.PrimitiveType(4, 'float32');
  self.float64 = new self.PrimitiveType(8, 'float64');
  // TODO alias size_t => uint32?
  self.size_t = new self.PrimitiveType(4, 'size_t');

  // TODO should this be created by default?
  self.uint8_p = new self.PointerType(self.uint8);

  var mallocType = new self.FunctionType(self.void_p, self.size_t);
  var memsetType = new self.FunctionType(self.void, self.void_p, self.int32, self.size_t);
  var memcpyType = new self.FunctionType(self.void, self.void_p, self.void_p, self.size_t);

  // Built-in functions.
  self.malloc = new self.CFunction('malloc', mallocType);
  self.memset = new self.CFunction('memset', memsetType);
  self.memcpy = new self.CFunction('memcpy', memcpyType);

  // NaCl stuff...
  var nextCallbackId = 1;
  var idCallbackMap = [];
  var moduleEl;

  function postMessage(msg, callback) {
    var id = nextCallbackId++;
    idCallbackMap[id] = callback;

    msg.id = id;
    if (moduleLoaded) {
      moduleEl.postMessage(msg);
    } else {
      queued.push(msg);
    }
    return id;
  }

  function postQueuedMessages() {
    queued.forEach(function(msg) {
      moduleEl.postMessage(msg);
    });

    queued = [];
  }

  function onModuleLoad(event) {
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

}).call(nacl);
