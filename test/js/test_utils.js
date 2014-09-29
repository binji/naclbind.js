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

var assert = require('chai').assert,
    utils = require('../../src/js/utils.js');

describe('Utils', function() {
  describe('getClass', function() {
    it('should work for numbers', function() {
      assert.equal(utils.getClass(0), 'Number');
      assert.equal(utils.getClass(-100), 'Number');
      assert.equal(utils.getClass(4.5), 'Number');
      assert.equal(utils.getClass(new Number(10)), 'Number');
      assert.equal(utils.getClass(1e34), 'Number');
    });

    it('should work for strings', function() {
      assert.equal(utils.getClass(''), 'String');
      assert.equal(utils.getClass('foo'), 'String');
      assert.equal(utils.getClass(new String('hello')), 'String');
    });

    it('should work for arrays', function() {
      assert.equal(utils.getClass([]), 'Array');
      assert.equal(utils.getClass([1, 2, 3]), 'Array');
      assert.equal(utils.getClass(new Array(10)), 'Array');
    });

    it('should work for objects', function() {
      function Dummy() {}
      assert.equal(utils.getClass({}), 'Object');
      assert.equal(utils.getClass({1: 2}), 'Object');
      assert.equal(utils.getClass(new Dummy()), 'Object');
    });
  });

  describe('isInteger', function() {
    it('should return true for the range -2**31..2**31-1', function() {
      assert.equal(utils.isInteger(utils.S32_MIN), true);
      assert.equal(utils.isInteger(-1e9), true);
      assert.equal(utils.isInteger(-1000), true);
      assert.equal(utils.isInteger(0), true);
      assert.equal(utils.isInteger(42), true);
      assert.equal(utils.isInteger(100.0), true);
      assert.equal(utils.isInteger(1e9), true);
      assert.equal(utils.isInteger(utils.S32_MAX), true);
    });

    it('should return false for floats', function() {
      assert.equal(utils.isInteger(4.5), false);
      assert.equal(utils.isInteger(Infinity), false);
      assert.equal(utils.isInteger(NaN), false);
      // These are integers, but they don't fit in an s32
      assert.equal(utils.isInteger(utils.S32_MAX + 1), false);
      assert.equal(utils.isInteger(1e20), false);
    });
  });

  describe('isFloat', function() {
    it('should return true for integers, abs(x) <= 2**24', function() {
      // += 100 to keep the test fast
      for (var x = -(1<<24); x <= 1<<24; x += 100) {
        assert.equal(utils.isFloat(x), true);
      }
      assert.equal(utils.isFloat(-(1<<24)-1), false);
      assert.equal(utils.isFloat((1<<24)+1), false);
    });

    it('should return true for large power-of-two integers', function() {
      assert.equal(utils.isFloat(1073741824), true);           // 2**30
      assert.equal(utils.isFloat(1099511627776), true);        // 2**40
      assert.equal(utils.isFloat(1125899906842624), true);     // 2**50
      assert.equal(utils.isFloat(1152921504606846976), true);  // 2**60
    });

    it('should return true for floats with < 23 bits mantissa', function() {
      assert.equal(utils.isFloat(4.5), true);
      assert.equal(utils.isFloat(100.125), true);
      assert.equal(utils.isFloat((1<<23)-0.5), true);
      assert.equal(utils.isFloat((1<<24)-0.5), false);
    });

    it('should return true for +-Inf and NaN', function() {
      assert.equal(utils.isFloat(Infinity), true);
      assert.equal(utils.isFloat(-Infinity), true);
      assert.equal(utils.isFloat(NaN), true);
    });
  });
});
