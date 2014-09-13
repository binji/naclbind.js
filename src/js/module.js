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

var type = require('./type'),
    utils = require('./utils');

function numberToType(n) {
  if (!isFinite(n) || !utils.isInteger(n)) {
    if (utils.isFloat(n)) {
      return type.float;
    } else {
      return type.double;
    }
  } else if (n < 0) {
    // Use the smallest integer type possible.
    if (n >= utils.S8_MIN) {
      return type.schar;
    } else if (n >= utils.S16_MIN) {
      return type.short;
    } else {
      return type.int;
    }
  } else {
    if (n <= utils.S8_MAX) {
      return type.schar;
    } else if (n <= utils.U8_MAX) {
      return type.uchar;
    } else if (n <= utils.S16_MAX) {
      return type.short;
    } else if (n <= utils.U16_MAX) {
      return type.ushort;
    } else {
      return type.int;
    }
  }
}

function objectToType(obj) {
  var klass = utils.getClass(obj);
  switch (klass) {
    case 'Number':
      return numberToType(obj);
    case 'String':
      return type.Pointer(type.char.qualify(type.CONST));
    // TODO(binji): handle other JS types.
    default:
      throw new Error('Unknown JavaScript class: "' + klass + '".');
  }
}

function objectToHandle(context, obj, type) {
  if (type === undefined) {
    type = objectToType(obj);
  } else {
    // TODO(binji): check that obj and type are compatible.
  }

  return context.createHandle(type, obj);
}

function argsToHandles(context, args) {
  return Array.prototype.map.call(args, function(arg) {
    return (arg instanceof Handle) ?  arg : objectToHandle(context, arg);
  });
}

function handlesToIds(handles) {
  return Array.prototype.map.call(handles, function(h) { return h.id; });
}

function Module(embed) {
  if (!(this instanceof Module)) { return new Module(embed); }
  this.$embed_ = embed || null;
  this.$handles_ = new HandleList();
  this.$functions_ = {};
  this.$context = this.$createContext();
  this.$initMessage_();
}
Module.prototype.$defineFunction = function(name, functions) {
  utils.checkArray(functions, IdFunction);

  if (this.$functions_[name] !== undefined) {
    throw new Error('Function named "' + name + '" is already defined.');
  }

  var self = this,
      getType = function(x) { return x.type; },
      fnTypes = Array.prototype.map.call(functions, getType);

  this.$functions_[name] = functions;
  this[name] = function() {
    var argHandles = argsToHandles(self.$context, arguments),
        argTypes = argHandles.map(getType),
        bestFnIdx = type.getBestViableFunction(fnTypes, argTypes),
        fn,
        retHandle;

    if (bestFnIdx < 0) {
      // TODO(binji): print nice error here.
      throw new Error('Call to "' + name + '" failed. ArgTypes: ' +
                      argTypes.map(type.getSpelling).join(', '));
    }

    fn = functions[bestFnIdx];
    if (fn.type.resultType !== type.void) {
      retHandle = self.$context.createHandle(fn.type.resultType);
    }

    self.$registerHandlesWithValues_(argHandles);
    self.$pushCommand_(fn.id, argHandles, retHandle);

    return retHandle;
  };
};
Module.prototype.$createContext = function() {
  return new Context(this.$handles_);
};
Module.prototype.$initMessage_ = function() {
  this.$message_ = {getHandles: [], setHandles: {}, commands:[]};
};
Module.prototype.$getMessage = function() {
  return this.$message_;
};
Module.prototype.$handle = function(value, type) {
  var handle = objectToHandle(this.$context, value, type);
  this.$registerHandleWithValue_(handle);
  return handle;
};
Module.prototype.$registerHandleWithValue_ = function(handle) {
  if (handle.value === null) {
    return;
  }

  this.$message_.setHandles[handle.id] = handle.value;
};
Module.prototype.$registerHandlesWithValues_ = function(handles) {
  var i;
  for (i = 0; i < handles.length; ++i) {
    this.$registerHandleWithValue_(handles[i]);
  }
};
Module.prototype.$pushCommand_ = function(id, argHandles, retHandle) {
  var command = {
    id: id,
    args: handlesToIds(argHandles)
  };

  if (retHandle) {
    command.ret = retHandle.id;
  }

  this.$message_.commands.push(command);
};
Module.prototype.$commit = function(handles, callback) {
  var self = this,
      context = this.$context;
  this.$message_.getHandles = handlesToIds(handles);
  this.$embed_.postMessage(this.$message_, function(msg) {
    // Call the callback with the same context as was set when $commit() was
    // called, then reset to the previous value.
    var oldContext = self.$context;
    self.$context = context;
    callback.apply(null, msg.values);
    self.$context = oldContext;
  });
  this.$initMessage_();
};

function IdFunction(id, fnType) {
  if (!(this instanceof IdFunction)) { return new IdFunction(id, fnType); }
  utils.checkNonnegativeNumber(id);
  type.checkType(fnType, 'type', type.FUNCTIONPROTO);

  this.id = id;
  this.type = fnType;
}

function HandleList() {
  this.nextId_ = 1;
  this.idToHandle_ = {};
}
HandleList.prototype.createHandle = function(context, type, value, id) {
  if (id === undefined) {
    id = this.nextId_++;
  }

  return new Handle(context, type, value, id);
};
HandleList.prototype.get = function(id) {
  return this.idToHandle_[id];
};
HandleList.prototype.registerHandle = function(handle) {
  this.idToHandle_[handle.id] = handle;
};

function Context(handleList) {
  this.handleList = handleList;
  this.handles = [];
}
Context.prototype.createHandle = function(type, value, id) {
  return this.handleList.createHandle(this, type, value, id);
};
Context.prototype.registerHandle = function(handle) {
  this.handleList.registerHandle(handle);
  this.handles.push(handle);
};

function Handle(context, type, value, id) {
  this.id = id;
  this.type = type;
  this.value = value || null;
  this.context = context;
  this.context.registerHandle(this);
}
Handle.prototype.cast = function(type) {
  var castResult = this.type.canCastTo(type);
  if (castResult === type.CAST_ERROR) {
    throw new Error('Invalid cast: ' + this.type.spelling + ' to ' +
                    type.spelling + '.');
  }

  return this.context.handleList.createHandle(this.context, type, this.value,
                                              this.id);
};


module.exports = {
  Module: Module,
  Function: IdFunction,

  numberToType: numberToType,
  objectToType: objectToType,
};
