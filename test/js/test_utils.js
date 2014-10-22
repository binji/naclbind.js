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

var chai = require('chai');
var assert = chai.assert;
var utils = require('../../src/js/naclbind').utils;

chai.config.includeStack = true;

describe('Utils', function() {
  describe('getClass', function() {
    it('should work for numbers', function() {
      assert.strictEqual(utils.getClass(0), 'Number');
      assert.strictEqual(utils.getClass(-100), 'Number');
      assert.strictEqual(utils.getClass(4.5), 'Number');
      assert.strictEqual(utils.getClass(new Number(10)), 'Number');
      assert.strictEqual(utils.getClass(1e34), 'Number');
    });

    it('should work for strings', function() {
      assert.strictEqual(utils.getClass(''), 'String');
      assert.strictEqual(utils.getClass('foo'), 'String');
      assert.strictEqual(utils.getClass(new String('hello')), 'String');
    });

    it('should work for arrays', function() {
      assert.strictEqual(utils.getClass([]), 'Array');
      assert.strictEqual(utils.getClass([1, 2, 3]), 'Array');
      assert.strictEqual(utils.getClass(new Array(10)), 'Array');
    });

    it('should work for objects', function() {
      function Dummy() {}
      assert.strictEqual(utils.getClass({}), 'Object');
      assert.strictEqual(utils.getClass({1: 2}), 'Object');
      assert.strictEqual(utils.getClass(new Dummy()), 'Object');
    });
  });

  describe('isInteger', function() {
    it('should return true for the range -2**31..2**31-1', function() {
      assert.strictEqual(utils.isInteger(utils.S32_MIN), true);
      assert.strictEqual(utils.isInteger(-1e9), true);
      assert.strictEqual(utils.isInteger(-1000), true);
      assert.strictEqual(utils.isInteger(0), true);
      assert.strictEqual(utils.isInteger(42), true);
      assert.strictEqual(utils.isInteger(100.0), true);
      assert.strictEqual(utils.isInteger(1e9), true);
      assert.strictEqual(utils.isInteger(utils.S32_MAX), true);
    });

    it('should return false for floats', function() {
      assert.strictEqual(utils.isInteger(4.5), false);
      assert.strictEqual(utils.isInteger(Infinity), false);
      assert.strictEqual(utils.isInteger(NaN), false);
      // These are integers, but they don't fit in an s32
      assert.strictEqual(utils.isInteger(utils.S32_MAX + 1), false);
      assert.strictEqual(utils.isInteger(1e20), false);
    });
  });

  describe('isFloat', function() {
    it('should return true for integers, abs(x) <= 2**24', function() {
      var x;
      // += 100 to keep the test fast
      for (x = -(1<<24); x <= 1<<24; x += 100) {
        assert.strictEqual(utils.isFloat(x), true);
      }
      assert.strictEqual(utils.isFloat(-(1<<24)-1), false);
      assert.strictEqual(utils.isFloat((1<<24)+1), false);
    });

    it('should return true for large power-of-two integers', function() {
      assert.strictEqual(utils.isFloat(1073741824), true);           // 2**30
      assert.strictEqual(utils.isFloat(1099511627776), true);        // 2**40
      assert.strictEqual(utils.isFloat(1125899906842624), true);     // 2**50
      assert.strictEqual(utils.isFloat(1152921504606846976), true);  // 2**60
    });

    it('should return true for floats with < 23 bits mantissa', function() {
      assert.strictEqual(utils.isFloat(4.5), true);
      assert.strictEqual(utils.isFloat(100.125), true);
      assert.strictEqual(utils.isFloat((1<<23)-0.5), true);
      assert.strictEqual(utils.isFloat((1<<24)-0.5), false);
    });

    it('should return true for +-Inf and NaN', function() {
      assert.strictEqual(utils.isFloat(Infinity), true);
      assert.strictEqual(utils.isFloat(-Infinity), true);
      assert.strictEqual(utils.isFloat(NaN), true);
    });
  });
});
