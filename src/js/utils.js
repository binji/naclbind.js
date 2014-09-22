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

function compose(f, g) {
  return function(x) {
    return g(f(x));
  };
}

function everyArrayPair(a1, a2, f) {
  var i;

  if (a1.length !== a2.length) {
    return false;
  }

  for (i = 0; i < a1.length; ++a1) {
    if (!f(a1[i], a2[i])) {
      return false;
    }
  }

  return true;
}

function getClass(x) {
  //  012345678
  // "[Object xxx]"
  var s = Object.prototype.toString.call(x);
  return s.substring(8, s.length - 1);
}

function checkNullOrString(x, varName) {
  if (!(x === null || getClass(x) === 'String')) {
    throw new Error(varName + ' must be null or string.');
  }
}

function checkArray(x, elementType, varName) {
  if (getClass(x) !== 'Array') {
    throw new Error(varName + ' must be an array.');
  }

  if (!(x.every(function(el) { return el instanceof elementType; }))) {
    throw new Error(varName + ' must be an array of type ' + elementType.name);
  }
}

function checkNumber(x, varName) {
  if (getClass(x) !== 'Number') {
    throw new Error(varName + ' must be a number.');
  }
}

function checkNonnegativeNumber(x, varName) {
  checkNumber(x, varName);
  if (x < 0) {
    throw new Error(varName + ' must be greater than 0.');
  }
}

function isNumber(n) {
  return n === +n;
}

function isInteger(n) {
  return n === (n | 0);
}

function isFloat(n) {
  if (!isFinite(n)) {
    return true;
  }

  isFloat.buffer[0] = n;
  return n === isFloat.buffer[0];
}
isFloat.buffer = new Float32Array(1);


module.exports = {
  checkArray: checkArray,
  checkNumber: checkNumber,
  checkNonnegativeNumber: checkNonnegativeNumber,
  checkNullOrString: checkNullOrString,
  compose: compose,
  everyArrayPair: everyArrayPair,
  getClass: getClass,
  isNumber: isNumber,
  isInteger: isInteger,
  isFloat: isFloat,

  S8_MIN: -128,
  S8_MAX: 127,
  U8_MAX: 255,
  S16_MIN: -32768,
  S16_MAX: 32767,
  U16_MAX: 65535,
  S32_MIN: -2147483648,
  S32_MAX: 2147483647,
  U32_MAX: 4294967295,
};

