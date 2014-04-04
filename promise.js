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

var promise={};
(function() {
  var self = this;

  function ArgumentsWrapper(args) {
    this.args = args;
  }

  function wrapArguments(args) {
    return new ArgumentsWrapper(args);
  }

  function wrapPromise(p) {
    if (p instanceof PromisePlus) {
      return p;
    }
    return new PromisePlus(p);
  }

  function unwrapPromise(p) {
    if (p instanceof PromisePlus) {
      return p.promise;
    }

    return p;
  }

  function PromisePlus(f) {
    if (f instanceof Promise) {
      this.promise = f;
    } else if ((f instanceof Function) && f.length === 3) {
      // Function that uses the "resolveMany" callback.
      this.promise = new Promise(function(resolve, reject) {
        f(function(x) { return resolve(x); },
          function(x) { return reject(x); },
          function() { return resolve(wrapArguments(arguments)); });
      });
    } else {
      this.promise = new Promise(f);
    }
  }
  PromisePlus.prototype = Object.create(Promise.prototype);
  PromisePlus.prototype.constructor = PromisePlus;

  PromisePlus.resolve = function(x) {
    return new PromisePlus(function(resolve) { resolve(x); });
  };

  PromisePlus.reject = function(x) {
    return new PromisePlus(function(resolve, reject) { reject(x); });
  };

  PromisePlus.prototype.then = function(resolve, reject) {
    return wrapPromise(this.promise.then(function(value) {
      if (value instanceof ArgumentsWrapper) {
        return unwrapPromise(resolve.apply(null, value.args));
      } else {
        return unwrapPromise(resolve(value));
      }
    }, function(value) {
      return unwrapPromise(reject(value));
    }));
  };

  PromisePlus.prototype.catch = function(reject) {
    return wrapPromise(this.promise.catch(function(value) {
      return unwrapPromise(reject(value));
    }));
  };

  PromisePlus.prototype.finally = function(f) {
    return this.then(function() {
      var args = arguments;
      return resolve().then(f).then(function() {
        return resolveMany.apply(null, args);
      });
    }, function() {
      return resolve().then(f).then(function() { return reject(x); });
    });
  };

  PromisePlus.prototype.if = function(cond, trueBlock, falseBlock) {
    return this.then(function() {
      var resolvedArgs = resolveMany.apply(null, arguments);
      return resolvedArgs.then(cond).then(function(condResult) {
        if (condResult) {
          return resolvedArgs.then(trueBlock);
        } else {
          if (falseBlock) {
            return resolvedArgs.then(falseBlock);
          } else {
            return undefined;
          }
        }
      });
    });
  };

  PromisePlus.prototype.while = function(cond, block) {
    return this.then(function loop() {
      var resolvedArgs = resolveMany.apply(null, arguments);
      return resolvedArgs.then(cond).then(function(condResult) {
        if (condResult) {
          return resolvedArgs.then(block).then(function() {
            return loop.apply(null, arguments);
          });
        } else {
          return resolvedArgs;
        }
      });
    });
  };

  function reject(value) {
    return PromisePlus.reject(value);
  }

  function resolve(value) {
    return PromisePlus.resolve(value);
  }

  function resolveMany() {
    return PromisePlus.resolve(wrapArguments(arguments));
  }

  // exported constructors
  self.PromisePlus = PromisePlus;

  // exported functions
  self.reject = reject;
  self.resolve = resolve;
  self.resolveMany = resolveMany;
}).call(promise);
