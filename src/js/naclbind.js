/* Copyright 2014 Ben Smith. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Long code is from by Google Closure Long (see
// https://code.google.com/p/closure-library/source/browse/closure/goog/math/long.js)
// with the following copyright:
//
// Copyright 2009 The Closure Library Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License./

/** STRIP_START ***************************************************************/

// UMD-style loader copied from:
// https://github.com/umdjs/umd/blob/master/returnExports.js
(function (root, factory) {
  /* jshint undef: true */
  /* global define */
  if (typeof define === 'function' && define.amd) {
    // AMD. Register as an anonymous module.
    define([], factory);
  } else if (typeof exports === 'object') {
    // Node. Does not work with strict CommonJS, but
    // only CommonJS-like environments that support module.exports,
    // like Node.
    module.exports = factory();
  } else {
    // Browser globals (root is window)
    root.naclbind = factory();
  }
}(this, function () {

/** STRIP_END *****************************************************************/

// utils ///////////////////////////////////////////////////////////////////////
var utils = (function() {

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
      throw new Error(varName + ' must be an array of type ' +
                      elementType.name);
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

  function isUnsignedInteger(n) {
    return n === (n >>> 0);
  }

  function isFloat(n) {
    if (!isFinite(n)) {
      return true;
    }

    isFloat.buffer[0] = n;
    return n === isFloat.buffer[0];
  }
  isFloat.buffer = new Float32Array(1);


  return {
    checkArray: checkArray,
    checkNumber: checkNumber,
    checkNonnegativeNumber: checkNonnegativeNumber,
    checkNullOrString: checkNullOrString,
    compose: compose,
    everyArrayPair: everyArrayPair,
    getClass: getClass,
    isNumber: isNumber,
    isInteger: isInteger,
    isUnsignedInteger: isUnsignedInteger,
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
})();

// embed ///////////////////////////////////////////////////////////////////////
var Embed = (function(utils) {

  function Embed(naclEmbed) {
    if (!(this instanceof Embed)) {
      return new Embed(naclEmbed);
    }
    this.embed_ = naclEmbed;

    this.queuedMessages_ = [];
    this.embed_.addLoadListener(this.onLoad_.bind(this));
    this.embed_.addMessageListener(this.onMessage_.bind(this));
    this.loaded_ = false;

    this.nextId_ = 1;
    this.idCallbackMap_ = [];
  }

  Embed.prototype.onLoad_ = function(e) {
    // Wait till the next time through the eventloop to allow other 'load'
    // listeners to be called.
    var self = this,
        nextTick = typeof process !== 'undefined' ?
            process.nextTick :
            window.setTimeout;

    nextTick(self.postQueuedMessages_.bind(self));

    this.loaded_ = true;
  };

  Embed.prototype.onMessage_ = function(e) {
    var msg = e.data,
        jsonMsg,
        id,
        callback;

    if (typeof(msg) !== 'object') {
      jsonMsg = JSON.stringify(msg);
      throw new Error('Unexpected value from module: ' + jsonMsg);
    }

    id = msg.id;
    if (!(utils.isNumber(id) && utils.isInteger(id))) {
      jsonMsg = JSON.stringify(msg);
      throw new Error('Received message with bad id: ' + jsonMsg);
    }

    callback = this.idCallbackMap_[id];
    if (utils.getClass(callback) !== 'Function') {
      jsonMsg = JSON.stringify(msg);
      throw new Error('No callback associated with id: ' + id + ' for msg: ' +
                      jsonMsg);
    }

    callback(msg);
    delete this.idCallbackMap_[id];
  };

  Embed.prototype.postQueuedMessages_ = function() {
    var i;
    for (i = 0; i < this.queuedMessages_.length; ++i) {
      this.embed_.postMessage(this.queuedMessages_[i]);
    }
    this.queuedMessages_ = null;
  };

  Embed.prototype.postMessage = function(msg, callback) {
    var id = this.nextId_++;

    this.idCallbackMap_[id] = callback;
    msg.id = id;

    if (!this.loaded_) {
      this.queuedMessages_.push(msg);
      return;
    }

    this.embed_.postMessage(msg);
  };

  Embed.prototype.appendToBody = function() {
    this.embed_.appendToBody();
  };

  return Embed;

})(utils);

// long ////////////////////////////////////////////////////////////////////////
var Long = (function() {
  function Long(low, high) {
    if (!(this instanceof Long)) { return new Long(low, high); }
    this.low_ = low | 0;
    this.high_ = high | 0;
  }

  Long.IntCache_ = {};

  Long.fromInt = function(value) {
    if (-128 <= value && value < 128) {
      var cachedObj = Long.IntCache_[value];
      if (cachedObj) {
        return cachedObj;
      }
    }

    var obj = Long(value | 0, value < 0 ? -1 : 0);
    if (-128 <= value && value < 128) {
      Long.IntCache_[value] = obj;
    }
    return obj;
  };

  Long.fromNumber = function(value) {
    if (isNaN(value) || !isFinite(value)) {
      return Long.ZERO;
    } else if (value <= -Long.TWO_PWR_63_DBL_) {
      return Long.MIN_VALUE;
    } else if (value + 1 >= Long.TWO_PWR_63_DBL_) {
      return Long.MAX_VALUE;
    } else if (value < 0) {
      return Long.fromNumber(-value).negate();
    } else {
      return Long(
          (value % Long.TWO_PWR_32_DBL_) | 0,
          (value / Long.TWO_PWR_32_DBL_) | 0);
    }
  };

  Long.fromBits = function(low, high) {
    return Long(low, high);
  };

  Long.TWO_PWR_16_DBL_ = 1 << 16;
  Long.TWO_PWR_32_DBL_ = Long.TWO_PWR_16_DBL_ * Long.TWO_PWR_16_DBL_;
  Long.TWO_PWR_64_DBL_ = Long.TWO_PWR_32_DBL_ * Long.TWO_PWR_32_DBL_;
  Long.TWO_PWR_63_DBL_ = Long.TWO_PWR_64_DBL_ / 2;
  Long.ZERO = Long.fromInt(0);
  Long.ONE = Long.fromInt(1);
  Long.NEG_ONE = Long.fromInt(-1);
  Long.MAX_VALUE = Long.fromBits(0xFFFFFFFF | 0, 0x7FFFFFFF | 0);
  Long.MIN_VALUE = Long.fromBits(0, 0x80000000 | 0);
  Long.TWO_PWR_24_ = Long.fromInt(1 << 24);

  /** @return {number} The value, assuming it is a 32-bit integer. */
  Long.prototype.toInt = function() {
    return this.low_;
  };


  /**
   * @return {number} The closest floating-point representation to this value.
   */
  Long.prototype.toNumber = function() {
    return this.high_ * Long.TWO_PWR_32_DBL_ +
           this.getLowBitsUnsigned();
  };


  /**
   * @param {number=} opt_radix The radix in which the text should be written.
   * @return {string} The textual representation of this value.
   * @override
   */
  Long.prototype.toString = function(opt_radix) {
    var radix = opt_radix || 10,
        rem;

    if (radix < 2 || 36 < radix) {
      throw Error('radix out of range: ' + radix);
    }

    if (this.isZero()) {
      return '0';
    }

    if (this.isNegative()) {
      if (this.equals(Long.MIN_VALUE)) {
        // We need to change the Long value before it can be negated, so we
        // remove the bottom-most digit in this base and then recurse to do the
        // rest.
        var radixLong = Long.fromNumber(radix);
        var div = this.div(radixLong);
        rem = div.multiply(radixLong).subtract(this);
        return div.toString(radix) + rem.toInt().toString(radix);
      } else {
        return '-' + this.negate().toString(radix);
      }
    }

    // Do several (6) digits each time through the loop, so as to
    // minimize the calls to the very expensive emulated div.
    var radixToPower = Long.fromNumber(Math.pow(radix, 6));

    rem = this;
    var result = '';
    while (true) {
      var remDiv = rem.div(radixToPower);
      var intval = rem.subtract(remDiv.multiply(radixToPower)).toInt();
      var digits = intval.toString(radix);

      rem = remDiv;
      if (rem.isZero()) {
        return digits + result;
      } else {
        while (digits.length < 6) {
          digits = '0' + digits;
        }
        result = '' + digits + result;
      }
    }
  };


  /** @return {number} The high 32-bits as a signed value. */
  Long.prototype.getHighBits = function() {
    return this.high_;
  };


  /** @return {number} The low 32-bits as a signed value. */
  Long.prototype.getLowBits = function() {
    return this.low_;
  };


  /** @return {number} The low 32-bits as an unsigned value. */
  Long.prototype.getLowBitsUnsigned = function() {
    return (this.low_ >= 0) ?
        this.low_ : Long.TWO_PWR_32_DBL_ + this.low_;
  };


  /**
   * @return {number} Returns the number of bits needed to represent the
   *     absolute value of this Long.
   */
  Long.prototype.getNumBitsAbs = function() {
    if (this.isNegative()) {
      if (this.equals(Long.MIN_VALUE)) {
        return 64;
      } else {
        return this.negate().getNumBitsAbs();
      }
    } else {
      var val = this.high_ !== 0 ? this.high_ : this.low_;
      for (var bit = 31; bit > 0; bit--) {
        if ((val & (1 << bit)) !== 0) {
          break;
        }
      }
      return this.high_ !== 0 ? bit + 33 : bit + 1;
    }
  };


  /** @return {boolean} Whether this value is zero. */
  Long.prototype.isZero = function() {
    return this.high_ === 0 && this.low_ === 0;
  };


  /** @return {boolean} Whether this value is negative. */
  Long.prototype.isNegative = function() {
    return this.high_ < 0;
  };


  /** @return {boolean} Whether this value is odd. */
  Long.prototype.isOdd = function() {
    return (this.low_ & 1) === 1;
  };


  /**
   * @param {Long} other Long to compare against.
   * @return {boolean} Whether this Long equals the other.
   */
  Long.prototype.equals = function(other) {
    return (this.high_ === other.high_) && (this.low_ === other.low_);
  };


  /**
   * @param {Long} other Long to compare against.
   * @return {boolean} Whether this Long does not equal the other.
   */
  Long.prototype.notEquals = function(other) {
    return (this.high_ !== other.high_) || (this.low_ !== other.low_);
  };


  /**
   * @param {Long} other Long to compare against.
   * @return {boolean} Whether this Long is less than the other.
   */
  Long.prototype.lessThan = function(other) {
    return this.compare(other) < 0;
  };


  /**
   * @param {Long} other Long to compare against.
   * @return {boolean} Whether this Long is less than or equal to the other.
   */
  Long.prototype.lessThanOrEqual = function(other) {
    return this.compare(other) <= 0;
  };


  /**
   * @param {Long} other Long to compare against.
   * @return {boolean} Whether this Long is greater than the other.
   */
  Long.prototype.greaterThan = function(other) {
    return this.compare(other) > 0;
  };


  /**
   * @param {Long} other Long to compare against.
   * @return {boolean} Whether this Long is greater than or equal to the other.
   */
  Long.prototype.greaterThanOrEqual = function(other) {
    return this.compare(other) >= 0;
  };


  /**
   * Compares this Long with the given one.
   * @param {Long} other Long to compare against.
   * @return {number} 0 if they are the same, 1 if the this is greater, and -1
   *     if the given one is greater.
   */
  Long.prototype.compare = function(other) {
    if (this.equals(other)) {
      return 0;
    }

    var thisNeg = this.isNegative();
    var otherNeg = other.isNegative();
    if (thisNeg && !otherNeg) {
      return -1;
    }
    if (!thisNeg && otherNeg) {
      return 1;
    }

    // at this point, the signs are the same, so subtraction will not overflow
    if (this.subtract(other).isNegative()) {
      return -1;
    } else {
      return 1;
    }
  };


  /** @return {!Long} The negation of this value. */
  Long.prototype.negate = function() {
    if (this.equals(Long.MIN_VALUE)) {
      return Long.MIN_VALUE;
    } else {
      return this.not().add(Long.ONE);
    }
  };


  /**
   * Returns the sum of this and the given Long.
   * @param {Long} other Long to add to this one.
   * @return {!Long} The sum of this and the given Long.
   */
  Long.prototype.add = function(other) {
    // Divide each number into 4 chunks of 16 bits, and then sum the chunks.

    var a48 = this.high_ >>> 16;
    var a32 = this.high_ & 0xFFFF;
    var a16 = this.low_ >>> 16;
    var a00 = this.low_ & 0xFFFF;

    var b48 = other.high_ >>> 16;
    var b32 = other.high_ & 0xFFFF;
    var b16 = other.low_ >>> 16;
    var b00 = other.low_ & 0xFFFF;

    var c48 = 0, c32 = 0, c16 = 0, c00 = 0;
    c00 += a00 + b00;
    c16 += c00 >>> 16;
    c00 &= 0xFFFF;
    c16 += a16 + b16;
    c32 += c16 >>> 16;
    c16 &= 0xFFFF;
    c32 += a32 + b32;
    c48 += c32 >>> 16;
    c32 &= 0xFFFF;
    c48 += a48 + b48;
    c48 &= 0xFFFF;
    return Long.fromBits((c16 << 16) | c00, (c48 << 16) | c32);
  };


  /**
   * Returns the difference of this and the given Long.
   * @param {Long} other Long to subtract from this.
   * @return {!Long} The difference of this and the given Long.
   */
  Long.prototype.subtract = function(other) {
    return this.add(other.negate());
  };


  /**
   * Returns the product of this and the given long.
   * @param {Long} other Long to multiply with this.
   * @return {!Long} The product of this and the other.
   */
  Long.prototype.multiply = function(other) {
    if (this.isZero()) {
      return Long.ZERO;
    } else if (other.isZero()) {
      return Long.ZERO;
    }

    if (this.equals(Long.MIN_VALUE)) {
      return other.isOdd() ? Long.MIN_VALUE : Long.ZERO;
    } else if (other.equals(Long.MIN_VALUE)) {
      return this.isOdd() ? Long.MIN_VALUE : Long.ZERO;
    }

    if (this.isNegative()) {
      if (other.isNegative()) {
        return this.negate().multiply(other.negate());
      } else {
        return this.negate().multiply(other).negate();
      }
    } else if (other.isNegative()) {
      return this.multiply(other.negate()).negate();
    }

    // If both longs are small, use float multiplication
    if (this.lessThan(Long.TWO_PWR_24_) &&
        other.lessThan(Long.TWO_PWR_24_)) {
      return Long.fromNumber(this.toNumber() * other.toNumber());
    }

    // Divide each long into 4 chunks of 16 bits, and then add up 4x4 products.
    // We can skip products that would overflow.

    var a48 = this.high_ >>> 16;
    var a32 = this.high_ & 0xFFFF;
    var a16 = this.low_ >>> 16;
    var a00 = this.low_ & 0xFFFF;

    var b48 = other.high_ >>> 16;
    var b32 = other.high_ & 0xFFFF;
    var b16 = other.low_ >>> 16;
    var b00 = other.low_ & 0xFFFF;

    var c48 = 0, c32 = 0, c16 = 0, c00 = 0;
    c00 += a00 * b00;
    c16 += c00 >>> 16;
    c00 &= 0xFFFF;
    c16 += a16 * b00;
    c32 += c16 >>> 16;
    c16 &= 0xFFFF;
    c16 += a00 * b16;
    c32 += c16 >>> 16;
    c16 &= 0xFFFF;
    c32 += a32 * b00;
    c48 += c32 >>> 16;
    c32 &= 0xFFFF;
    c32 += a16 * b16;
    c48 += c32 >>> 16;
    c32 &= 0xFFFF;
    c32 += a00 * b32;
    c48 += c32 >>> 16;
    c32 &= 0xFFFF;
    c48 += a48 * b00 + a32 * b16 + a16 * b32 + a00 * b48;
    c48 &= 0xFFFF;
    return Long.fromBits((c16 << 16) | c00, (c48 << 16) | c32);
  };


  /**
   * Returns this Long divided by the given one.
   * @param {Long} other Long by which to divide.
   * @return {!Long} This Long divided by the given one.
   */
  Long.prototype.div = function(other) {
    var rem,
        approx;

    if (other.isZero()) {
      throw Error('division by zero');
    } else if (this.isZero()) {
      return Long.ZERO;
    }

    if (this.equals(Long.MIN_VALUE)) {
      if (other.equals(Long.ONE) ||
          other.equals(Long.NEG_ONE)) {
        return Long.MIN_VALUE;  // recall that -MIN_VALUE == MIN_VALUE
      } else if (other.equals(Long.MIN_VALUE)) {
        return Long.ONE;
      } else {
        // At this point, we have |other| >= 2, so |this/other| < |MIN_VALUE|.
        var halfThis = this.shiftRight(1);
        approx = halfThis.div(other).shiftLeft(1);
        if (approx.equals(Long.ZERO)) {
          return other.isNegative() ? Long.ONE : Long.NEG_ONE;
        } else {
          rem = this.subtract(other.multiply(approx));
          var result = approx.add(rem.div(other));
          return result;
        }
      }
    } else if (other.equals(Long.MIN_VALUE)) {
      return Long.ZERO;
    }

    if (this.isNegative()) {
      if (other.isNegative()) {
        return this.negate().div(other.negate());
      } else {
        return this.negate().div(other).negate();
      }
    } else if (other.isNegative()) {
      return this.div(other.negate()).negate();
    }

    // Repeat the following until the remainder is less than other:  find a
    // floating-point that approximates remainder / other *from below*, add this
    // into the result, and subtract it from the remainder.  It is critical that
    // the approximate value is less than or equal to the real value so that the
    // remainder never becomes negative.
    var res = Long.ZERO;
    rem = this;
    while (rem.greaterThanOrEqual(other)) {
      // Approximate the result of division. This may be a little greater or
      // smaller than the actual value.
      approx = Math.max(1, Math.floor(rem.toNumber() / other.toNumber()));

      // We will tweak the approximate result by changing it in the 48-th digit
      // or the smallest non-fractional digit, whichever is larger.
      var log2 = Math.ceil(Math.log(approx) / Math.LN2);
      var delta = (log2 <= 48) ? 1 : Math.pow(2, log2 - 48);

      // Decrease the approximation until it is smaller than the remainder.
      // Note that if it is too large, the product overflows and is negative.
      var approxRes = Long.fromNumber(approx);
      var approxRem = approxRes.multiply(other);
      while (approxRem.isNegative() || approxRem.greaterThan(rem)) {
        approx -= delta;
        approxRes = Long.fromNumber(approx);
        approxRem = approxRes.multiply(other);
      }

      // We know the answer can't be zero... and actually, zero would cause
      // infinite recursion since we would make no progress.
      if (approxRes.isZero()) {
        approxRes = Long.ONE;
      }

      res = res.add(approxRes);
      rem = rem.subtract(approxRem);
    }
    return res;
  };


  /**
   * Returns this Long modulo the given one.
   * @param {Long} other Long by which to mod.
   * @return {!Long} This Long modulo the given one.
   */
  Long.prototype.modulo = function(other) {
    return this.subtract(this.div(other).multiply(other));
  };


  /** @return {!Long} The bitwise-NOT of this value. */
  Long.prototype.not = function() {
    return Long.fromBits(~this.low_, ~this.high_);
  };


  /**
   * Returns the bitwise-AND of this Long and the given one.
   * @param {Long} other The Long with which to AND.
   * @return {!Long} The bitwise-AND of this and the other.
   */
  Long.prototype.and = function(other) {
    return Long.fromBits(this.low_ & other.low_, this.high_ & other.high_);
  };


  /**
   * Returns the bitwise-OR of this Long and the given one.
   * @param {Long} other The Long with which to OR.
   * @return {!Long} The bitwise-OR of this and the other.
   */
  Long.prototype.or = function(other) {
    return Long.fromBits(this.low_ | other.low_, this.high_ | other.high_);
  };


  /**
   * Returns the bitwise-XOR of this Long and the given one.
   * @param {Long} other The Long with which to XOR.
   * @return {!Long} The bitwise-XOR of this and the other.
   */
  Long.prototype.xor = function(other) {
    return Long.fromBits(this.low_ ^ other.low_, this.high_ ^ other.high_);
  };


  /**
   * Returns this Long with bits shifted to the left by the given amount.
   * @param {number} numBits The number of bits by which to shift.
   * @return {!Long} This shifted to the left by the given amount.
   */
  Long.prototype.shiftLeft = function(numBits) {
    numBits &= 63;
    if (numBits === 0) {
      return this;
    } else {
      var low = this.low_;
      if (numBits < 32) {
        var high = this.high_;
        return Long.fromBits(
            low << numBits,
            (high << numBits) | (low >>> (32 - numBits)));
      } else {
        return Long.fromBits(0, low << (numBits - 32));
      }
    }
  };


  /**
   * Returns this Long with bits shifted to the right by the given amount.
   * @param {number} numBits The number of bits by which to shift.
   * @return {!Long} This shifted to the right by the given amount.
   */
  Long.prototype.shiftRight = function(numBits) {
    numBits &= 63;
    if (numBits === 0) {
      return this;
    } else {
      var high = this.high_;
      if (numBits < 32) {
        var low = this.low_;
        return Long.fromBits(
            (low >>> numBits) | (high << (32 - numBits)),
            high >> numBits);
      } else {
        return Long.fromBits(
            high >> (numBits - 32),
            high >= 0 ? 0 : -1);
      }
    }
  };


  /**
   * Returns this Long with bits shifted to the right by the given amount, with
   * zeros placed into the new leading bits.
   * @param {number} numBits The number of bits by which to shift.
   * @return {!Long} This shifted to the right by the given amount, with
   *     zeros placed into the new leading bits.
   */
  Long.prototype.shiftRightUnsigned = function(numBits) {
    numBits &= 63;
    if (numBits === 0) {
      return this;
    } else {
      var high = this.high_;
      if (numBits < 32) {
        var low = this.low_;
        return Long.fromBits(
            (low >>> numBits) | (high << (32 - numBits)),
            high >>> numBits);
      } else if (numBits === 32) {
        return Long.fromBits(high, 0);
      } else {
        return Long.fromBits(high >>> (numBits - 32), 0);
      }
    }
  };

  return Long;
})();

// nacl_embed //////////////////////////////////////////////////////////////////
var NaClEmbed = (function() {

  function NaClEmbed(nmf, mimeType) {
    if (!(this instanceof NaClEmbed)) { return new NaClEmbed(nmf, mimeType); }
    this.nmf = nmf;
    this.mimeType = mimeType;
    this.element = document.createElement('embed');
    this.element.setAttribute('width', '0');
    this.element.setAttribute('height', '0');
    this.element.setAttribute('src', this.nmf);
    this.element.setAttribute('type', this.mimeType);
  }

  NaClEmbed.prototype.addEventListener_ = function(message, callback) {
    this.element.addEventListener(message, callback, false);
  };

  NaClEmbed.prototype.addLoadListener = function(callback) {
    this.addEventListener_('load', callback);
  };

  NaClEmbed.prototype.addMessageListener = function(callback) {
    this.addEventListener_('message', callback);
  };

  NaClEmbed.prototype.addErrorListener = function(callback) {
    this.addEventListener_('error', callback);
  };

  NaClEmbed.prototype.addCrashListener = function(callback) {
    this.addEventListener_('crash', callback);
  };

  NaClEmbed.prototype.appendToBody = function() {
    document.body.appendChild(this.element);
  };

  NaClEmbed.prototype.postMessage = function(msg) {
    this.element.postMessage(msg);
  };

  Object.defineProperty(NaClEmbed.prototype, 'lastError', {
    get: function() { return this.element.lastError; },
    enumerable: true
  });

  Object.defineProperty(NaClEmbed.prototype, 'exitStatus', {
    get: function() { return this.element.exitStatus; },
    enumerable: true
  });

  return NaClEmbed;
})();

// type ////////////////////////////////////////////////////////////////////////
var type = (function(utils) {

  // Matches LLVM TypeKind names (but not values).
  var INVALID = 0,
      UNEXPOSED = 1,
      VOID = 2,
      BOOL = 3,
      CHAR_U = 4,
      UCHAR = 5,
      USHORT = 6,
      UINT = 7,
      ULONG = 8,
      ULONGLONG = 9,
      CHAR_S = 10,
      SCHAR = 11,
      WCHAR = 12,
      SHORT = 13,
      INT = 14,
      LONG = 15,
      LONGLONG = 16,
      FLOAT = 17,
      DOUBLE = 18,
      LONGDOUBLE = 19,
      POINTER = 20,
      RECORD = 21,
      ENUM = 22,
      TYPEDEF = 23,
      FUNCTIONPROTO = 24,
      FUNCTIONNOPROTO = 25,
      CONSTANTARRAY = 26,
      INCOMPLETEARRAY = 27,

      KIND_NAME = {
        0: 'INVALID',
        1: 'UNEXPOSED',
        2: 'VOID',
        3: 'BOOL',
        4: 'CHAR_U',
        5: 'UCHAR',
        6: 'USHORT',
        7: 'UINT',
        8: 'ULONG',
        9: 'ULONGLONG',
        10: 'CHAR_S',
        11: 'SCHAR',
        12: 'WCHAR',
        13: 'SHORT',
        14: 'INT',
        15: 'LONG',
        16: 'LONGLONG',
        17: 'FLOAT',
        18: 'DOUBLE',
        19: 'LONGDOUBLE',
        20: 'POINTER',
        21: 'RECORD',
        22: 'ENUM',
        23: 'TYPEDEF',
        24: 'FUNCTIONPROTO',
        25: 'FUNCTIONNOPROTO',
        26: 'CONSTANTARRAY',
        27: 'INCOMPLETEARRAY'
      },

      PRIMITIVE_SPELLING = {
        2: 'void',
        3: 'bool',
        4: 'char',
        5: 'unsigned char',
        6: 'unsigned short',
        7: 'unsigned int',
        8: 'unsigned long',
        9: 'unsigned long long',
        10: 'char',
        11: 'signed char',
        12: 'wchar_t',
        13: 'short',
        14: 'int',
        15: 'long',
        16: 'long long',
        17: 'float',
        18: 'double',
        19: 'long double',
      },

      PRIMITIVE_RANK = {
        3: 1,  // bool
        4: 2,  // char (unsigned)
        5: 2,  // unsigned char
        6: 3,  // unsigned short
        7: 4,  // unsigned int
        8: 5,  // unsigned long
        9: 6,  // unsigned long long
        10: 2,  // char (signed)
        11: 2,  // signed char
        12: 4,  // wchar_t
        13: 3,  // short
        14: 4,  // int
        15: 5,  // long
        16: 6,  // long long
        17: 7,  // float
        18: 8,  // double
        19: 9,  // long double
      },

      PRIMITIVE_SIGNED = {
        4: true,  // char (unsigned)
        5: true,  // unsigned char
        6: true,  // unsigned short
        7: true,  // unsigned int
        8: true,  // unsigned long
        9: true,  // unsigned long long
        10: false,  // char (signed)
        11: false,  // signed char
        12: false,  // wchar_t
        13: false,  // short
        14: false,  // int
        15: false,  // long
        16: false,  // long long
      },

      PRIMITIVE_SIZE = {
        3: 1,  // bool
        4: 1,  // char (unsigned)
        5: 1,  // unsigned char
        6: 2,  // unsigned short
        7: 4,  // unsigned int
        8: 4,  // unsigned long
        9: 8,  // unsigned long long
        10: 1,  // char (signed)
        11: 1,  // signed char
        12: 4,  // wchar_t
        13: 2,  // short
        14: 4,  // int
        15: 4,  // long
        16: 8,  // long long
        17: 4,  // float
        18: 8,  // double
        19: 10,  // long double
      },

      IS_CONST = 1,
      IS_VOLATILE = 2,
      IS_RESTRICT = 4,

      VARIADIC = true,
      NOT_VARIADIC = false,

      STRUCT = false,
      UNION = true,

      // Success
      CAST_OK_EXACT = 3,
      CAST_OK_PROMOTION = 2,
      CAST_OK_CONVERSION = 1,
      CAST_OK_DEFAULT_PROMOTION = 0,
      // Warning
      CAST_TRUNCATE = -1,
      CAST_SIGNED_UNSIGNED = -2,
      CAST_INT_TO_POINTER = -3,
      CAST_POINTER_TO_INT = -4,
      CAST_DISCARD_QUALIFIER = -5,
      CAST_INT_TO_ENUM = -6,
      CAST_DIFFERENT_ENUMS = -7,
      CAST_INCOMPATIBLE_POINTERS = -8,
      CAST_FUNCTION_POINTER_VOID_POINTER = -9,
      CAST_FUNCTION_POINTER_NOPROTO = -10,
      // Failure
      CAST_ERROR = -11,

      CALL_OK = 0,
      CALL_WARNING = -1,
      CALL_ERROR = -2,

      SPELLING_PRECEDENCE = {};

  SPELLING_PRECEDENCE[POINTER] = 1;
  SPELLING_PRECEDENCE[CONSTANTARRAY] = 2;
  SPELLING_PRECEDENCE[INCOMPLETEARRAY] = 2;
  SPELLING_PRECEDENCE[FUNCTIONPROTO] = 3;
  SPELLING_PRECEDENCE[FUNCTIONNOPROTO] = 3;


  function checkType(x, varName, optKind) {
    if (!(x instanceof Type)) {
      throw new Error(varName + ' must be instanceof Type.');
    }
    if (optKind) {
      if (Array.isArray(optKind)) {
        if (optKind.indexOf(x.kind) === -1) {
          throw new Error(
              varName + ' must be Type with kind in [' +
              optKind.map(function(x) { return KIND_NAME[x]; }).join(', ') +
              ']');
        }
      } else if (x.kind !== optKind) {
        throw new Error(varName + ' must be Type with kind ' +
                        KIND_NAME[optKind]);
      }
    }
  }

  function isVoid(type) {
    return type.kind === VOID;
  }

  function isInteger(type) {
    return type.kind >= BOOL && type.kind <= LONGLONG;
  }

  function isNumeric(type) {
    return type.kind >= BOOL && type.kind <= LONGDOUBLE;
  }

  function isPointerlike(type) {
    return type.kind === POINTER || type.kind === CONSTANTARRAY ||
           type.kind === INCOMPLETEARRAY;
  }

  function isArray(type) {
    return type.kind === CONSTANTARRAY || type.kind === INCOMPLETEARRAY;
  }

  function isFunction(type) {
    return type.kind === FUNCTIONPROTO || type.kind === FUNCTIONNOPROTO;
  }

  function isCastOK(result) {
    return result >= 0;
  }

  function isCastWarning(result) {
    return result < 0 && result > CAST_ERROR;
  }

  function isCastError(result) {
    return result === CAST_ERROR;
  }

  function hasTypedef(type) {
    switch (type.kind) {
      case TYPEDEF:
        return true;
      case POINTER:
        return hasTypedef(type.pointee);
      case CONSTANTARRAY:
        return hasTypedef(type.elementType);
      case INCOMPLETEARRAY:
        return hasTypedef(type.elementType);
      case FUNCTIONPROTO:
        return hasTypedef(type.resultType) ||
               Array.prototype.some.call(type.argTypes, hasTypedef);
      case FUNCTIONNOPROTO:
        return hasTypedef(type.resultType);
      default:
        return false;
    }
  }

  function getCanonicalHelper(type) {
    var recurse = getCanonicalHelper;
    switch (type.kind) {
      case TYPEDEF:
        return getCanonicalHelper(type.alias).qualify(type.cv);
      case POINTER:
        return Pointer(recurse(type.pointee), type.cv);
      case CONSTANTARRAY:
        return ConstantArray(recurse(type.elementType), type.arraySize,
                             type.cv);
      case INCOMPLETEARRAY:
        return IncompleteArray(recurse(type.elementType), type.cv);
      case FUNCTIONPROTO:
        return FunctionProto(recurse(type.resultType),
                             Array.prototype.map.call(type.argTypes, recurse),
                             type.variadic);
      case FUNCTIONNOPROTO:
        return FunctionNoProto(recurse(type.resultType));
      default:
        return type;
    }
  }

  function getCanonical(type) {
    // Optimization. Don't create a new type unless there is a typedef in the
    // type tree.
    if (!hasTypedef(type)) {
      return type;
    }

    return getCanonicalHelper(type);
  }

  function getPointerlikePointee(type) {
    if (type.kind === POINTER) {
      return type.pointee;
    } else if (type.kind === CONSTANTARRAY) {
      return type.elementType;
    } else if (type.kind === INCOMPLETEARRAY) {
      return type.elementType;
    }
    return null;
  }

  function isLessQualified(q1, q2) {
    return (q2 & ~q1) !== 0 && (q1 & q2) === q1;
  }

  function isLessOrEquallyQualified(q1, q2) {
    return isLessQualified(q1, q2) || q1 === q2;
  }

  function isMoreQualified(q1, q2) {
    return isLessQualified(q2, q1);
  }

  function isMoreOrEquallyQualified(q1, q2) {
    return isLessQualified(q2, q1) || q1 === q2;
  }

  function Type(kind, cv) {
    if (!(this instanceof Type)) { return new Type(kind, cv); }

    if (cv !== undefined) {
      utils.checkNonnegativeNumber(cv, 'cv');

      if (cv > (IS_CONST | IS_VOLATILE | IS_RESTRICT)) {
        throw new Error('cv value out of range: ' + cv);
      }
    }

    this.kind = kind;
    this.cv = cv || 0;
  }
  Type.prototype.qualify = function(cv) {
    return null;
  };
  Type.prototype.isCompatibleWith = function(that) {
    return isCompatibleWith(this, that);
  };
  Type.prototype.canCastTo = function(that) {
    return canCast(this, that);
  };

  function Void(cv) {
    if (!(this instanceof Void)) { return new Void(cv); }
    Type.call(this, VOID, cv);
    this.spelling = getSpelling(this);
  }
  Void.prototype = Object.create(Type.prototype);
  Void.prototype.constructor = Void;
  Void.prototype.size = 0;
  Void.prototype.qualify = function(cv) {
    return Void(this.cv | cv);
  };
  Void.prototype.unqualified = function() {
    return Void();
  };

  function Numeric(kind, cv) {
    if (!(this instanceof Numeric)) { return new Numeric(kind, cv); }
    Type.call(this, kind, cv);
    this.size = PRIMITIVE_SIZE[kind];
    this.spelling = getSpelling(this);
  }
  Numeric.prototype = Object.create(Type.prototype);
  Numeric.prototype.constructor = Numeric;
  Numeric.prototype.qualify = function(cv) {
    return Numeric(this.kind, this.cv | cv);
  };
  Numeric.prototype.unqualified = function() {
    return Numeric(this.kind);
  };

  function Pointer(pointee, cv) {
    if (!(this instanceof Pointer)) { return new Pointer(pointee, cv); }

    checkType(pointee, 'pointee');

    Type.call(this, POINTER, cv);
    this.pointee = pointee;
    this.spelling = getSpelling(this);
  }
  Pointer.prototype = Object.create(Type.prototype);
  Pointer.prototype.constructor = Pointer;
  Pointer.prototype.size = 4;
  Pointer.prototype.qualify = function(cv) {
    return Pointer(this.pointee, this.cv | cv);
  };
  Pointer.prototype.unqualified = function() {
    return Pointer(this.pointee);
  };

  function Record(tag, size, isUnion, cv) {
    if (!(this instanceof Record)) {
      return new Record(tag, size, isUnion, cv);
    }

    utils.checkNullOrString(tag, 'tag');
    utils.checkNumber(size, 'size');

    if (isUnion !== undefined && utils.getClass(isUnion) !== 'Boolean') {
      throw new Error('isUnion must be a Boolean.');
    }

    Type.call(this, RECORD, cv);
    this.tag = tag;
    this.size = size;
    this.fields = [];
    this.isUnion = isUnion || false;
    this.spelling = getSpelling(this);
  }
  Record.prototype = Object.create(Type.prototype);
  Record.prototype.constructor = Record;
  Record.prototype.qualify = function(cv) {
    var record = Record(this.tag, this.size, this.isUnion, this.cv | cv);
    record.fields = this.fields;
    return record;
  };
  Record.prototype.unqualified = function() {
    var record = Record(this.tag, this.size, this.isUnion);
    record.fields = this.fields;
    return record;
  };
  Record.prototype.addField = function(name, type, offset) {
    this.fields.push(Field(name, type, offset));
  };

  function Field(name, type, offset) {
    if (!(this instanceof Field)) { return new Field(name, type, offset); }

    utils.checkNullOrString(name, 'name');
    checkType(type, 'type');
    utils.checkNonnegativeNumber(offset, 'offset');

    this.name = name;
    this.type = type;
    this.offset = offset;
  }


  function Enum(tag, cv) {
    if (!(this instanceof Enum)) { return new Enum(tag, cv); }

    utils.checkNullOrString(tag, 'tag');

    Type.call(this, ENUM, cv);
    this.tag = tag;
    this.spelling = getSpelling(this);
  }
  Enum.prototype = Object.create(Type.prototype);
  Enum.prototype.constructor = Enum;
  Enum.prototype.size = 4;
  Enum.prototype.qualify = function(cv) {
    return Enum(this.tag, this.cv | cv);
  };
  Enum.prototype.unqualified = function() {
    return Enum(this.tag);
  };

  function Typedef(tag, alias, cv) {
    if (!(this instanceof Typedef)) { return new Typedef(tag, alias, cv); }
    Type.call(this, TYPEDEF, cv);
    this.tag = tag;
    this.alias = alias;
    this.spelling = getSpelling(this);
  }
  Typedef.prototype = Object.create(Type.prototype);
  Typedef.prototype.constructor = Typedef;
  Typedef.prototype.qualify = function(cv) {
    return Typedef(this.tag, this.alias, this.cv | cv);
  };
  Typedef.prototype.unqualified = function() {
    return Typedef(this.tag, this.alias);
  };
  Object.defineProperty(Typedef.prototype, 'size', {
    get: function() { return this.alias.size; }
  });

  function FunctionProto(resultType, argTypes, variadic) {
    if (!(this instanceof FunctionProto)) {
      return new FunctionProto(resultType, argTypes, variadic);
    }

    checkType(resultType, 'resultType');
    utils.checkArray(argTypes, Type, 'argTypes');

    if (isArray(getCanonical(resultType))) {
      throw new Error('Function return type cannot be an array. Got ' +
                      resultType.spelling);
    }

    if (variadic && argTypes.length === 0) {
      throw new Error('Cannot create variadic function with no arguments.');
    }

    if (Array.prototype.some.call(argTypes,
                                  utils.compose(getCanonical, isVoid))) {
      throw new Error('Function argument type cannot be void.');
    }

    Type.call(this, FUNCTIONPROTO, 0);
    this.resultType = resultType;
    this.argTypes = argTypes;
    this.variadic = variadic || false;
    this.spelling = getSpelling(this);
  }
  FunctionProto.prototype = Object.create(Type.prototype);
  FunctionProto.prototype.size = -1;
  FunctionProto.prototype.constructor = FunctionProto;
  FunctionProto.prototype.qualify = function(cv) {
    return this;
  };
  FunctionProto.prototype.unqualified = function() {
    return this;
  };
  FunctionProto.prototype.isViableForCall = function(argTypes) {
    return this.argTypes.length === argTypes.length ||
           (this.argTypes.length < argTypes.length && this.variadic);
  };

  function FunctionNoProto(resultType) {
    if (!(this instanceof FunctionNoProto)) {
      return new FunctionNoProto(resultType);
    }

    checkType(resultType, 'resultType');

    if (isArray(getCanonical(resultType))) {
      throw new Error('Function return type cannot be an array. Got ' +
                      resultType.spelling);
    }

    Type.call(this, FUNCTIONNOPROTO, 0);
    this.resultType = resultType;
    this.spelling = getSpelling(this);
  }
  FunctionNoProto.prototype = Object.create(Type.prototype);
  FunctionNoProto.prototype.constructor = FunctionNoProto;
  FunctionNoProto.prototype.size = -1;
  FunctionNoProto.prototype.qualify = function(cv) {
    return this;
  };
  FunctionNoProto.prototype.unqualified = function() {
    return this;
  };
  FunctionNoProto.prototype.isViableForCall = function(argTypes) {
    return true;
  };

  function ConstantArray(elementType, arraySize) {
    if (!(this instanceof ConstantArray)) {
      return new ConstantArray(elementType, arraySize);
    }

    checkType(elementType, 'elementType');
    utils.checkNonnegativeNumber(arraySize, 'arraySize');

    if (elementType.kind === VOID) {
      throw new Error('Cannot create an array of voids.');
    }

    Type.call(this, CONSTANTARRAY, 0);
    this.elementType = elementType;
    this.arraySize = arraySize;
    this.size = this.elementType.size * this.arraySize;
    this.spelling = getSpelling(this);
  }
  ConstantArray.prototype = Object.create(Type.prototype);
  ConstantArray.prototype.constructor = ConstantArray;
  ConstantArray.prototype.qualify = function(cv) {
    return this;
  };
  ConstantArray.prototype.unqualified = function() {
    return this;
  };

  function IncompleteArray(elementType) {
    if (!(this instanceof IncompleteArray)) {
      return new IncompleteArray(elementType);
    }

    checkType(elementType, 'elementType');

    if (elementType.kind === VOID) {
      throw new Error('Cannot create an array of voids.');
    }

    Type.call(this, INCOMPLETEARRAY, 0);
    this.elementType = elementType;
    this.spelling = getSpelling(this);
  }
  IncompleteArray.prototype = Object.create(Type.prototype);
  IncompleteArray.prototype.constructor = IncompleteArray;
  IncompleteArray.prototype.size = 4;
  IncompleteArray.prototype.qualify = function(cv) {
    return this;
  };
  IncompleteArray.prototype.unqualified = function() {
    return this;
  };

  function getQualifier(cv) {
    var result = '';
    if (cv & IS_CONST) result += 'const ';
    if (cv & IS_VOLATILE) result += 'volatile ';
    if (cv & IS_RESTRICT) result += 'restrict ';
    return result;
  }

  function describeQualifier(cv) {
    var a;
    if (cv) {
      a = [];
      if (cv & IS_CONST) a.push('const');
      if (cv & IS_VOLATILE) a.push('volatile');
      if (cv & IS_RESTRICT) a.push('restrict');
      return a.join(' ');
    } else {
      return 'none';
    }
  }

  function getSpelling(type, opt_name, opt_lastKind) {
    var prec,
        lastPrec,
        spelling,
        argsSpelling,
        name;

    spelling = getQualifier(type.cv);
    if (type.kind in PRIMITIVE_SPELLING) {
      spelling += PRIMITIVE_SPELLING[type.kind];
      if (opt_name) {
        spelling += ' ' + opt_name;
      }

      return spelling;
    }

    name = opt_name || '';
    prec = SPELLING_PRECEDENCE[type.kind];
    lastPrec = SPELLING_PRECEDENCE[opt_lastKind];

    if (prec && lastPrec && prec > lastPrec) {
      name = '(' + name + ')';
    }

    if (type.kind === TYPEDEF) {
      spelling += type.tag;
      if (name) {
        spelling += ' ' + name;
      }
    } else if (type.kind === POINTER) {
      name = '*' + spelling + name;
      spelling = getSpelling(type.pointee, name, POINTER);
    } else if (type.kind === ENUM) {
      spelling += 'enum ' + type.tag;
      if (name) {
        spelling += ' ' + name;
      }
    } else if (type.kind === RECORD) {
      if (type.isUnion) {
        spelling += 'union ' + type.tag;
      } else {
        spelling += 'struct ' + type.tag;
      }
      if (name) {
        spelling += ' ' + name;
      }
    } else if (type.kind === CONSTANTARRAY) {
      name += '[' + type.arraySize + ']';
      spelling = getSpelling(type.elementType, name, CONSTANTARRAY);
    } else if (type.kind === INCOMPLETEARRAY) {
      name += '[]';
      spelling = getSpelling(type.elementType, name, INCOMPLETEARRAY);
    } else if (type.kind === FUNCTIONPROTO) {
      name += '(';
      if (type.argTypes.length > 0) {
        argsSpelling = type.argTypes.map(function(a) {
          return getSpelling(a);
        });
        if (type.variadic) {
          argsSpelling.push('...');
        }
        name += argsSpelling.join(', ');
      } else {
        name += 'void';
      }
      name += ')';
      spelling = getSpelling(type.resultType, name, FUNCTIONPROTO);
    } else if (type.kind === FUNCTIONNOPROTO) {
      name += '()';
      spelling = getSpelling(type.resultType, name, FUNCTIONNOPROTO);
    } else {
      throw new Error('Unknown kind: ' + type.kind);
    }

    return spelling;
  }

  function canCast(from, to) {
    from = getCanonical(from);
    to = getCanonical(to);

    if (isNumeric(from)) {
      return canCastNumeric(from, to);
    }

    switch (from.kind) {
      case VOID:
        return to.kind === VOID ? CAST_OK_EXACT : CAST_ERROR;
      case POINTER:
      case CONSTANTARRAY:
      case INCOMPLETEARRAY:
        return canCastPointer(from, to);
      case RECORD:
        return from.kind === to.kind &&
               from.tag === to.tag &&
               from.isUnion === to.isUnion ?
            CAST_OK_EXACT :
            CAST_ERROR;
      case ENUM:
        if (isInteger(to)) {
          return CAST_OK_CONVERSION;
        } else if (to.kind === ENUM) {
          return from.tag === to.tag ? CAST_OK_EXACT : CAST_DIFFERENT_ENUMS;
        } else {
          return CAST_ERROR;
        }
        break;
      case FUNCTIONPROTO:
        return CAST_ERROR;
      case FUNCTIONNOPROTO:
        return CAST_ERROR;
      default:
        throw new Error('canCast: Unknown kind ' + from.kind);
    }
  }

  function canCastNumeric(from, to) {
    if (from.kind === to.kind) {
      return CAST_OK_EXACT;
    }

    if (isInteger(from)) {
      if (isPointerlike(to)) {
        return CAST_INT_TO_POINTER;
      } else if (to.kind === ENUM) {
        return CAST_INT_TO_ENUM;
      } else if (isNumeric(to)) {
        // Fall through to below.
      } else {
        return CAST_ERROR;
      }
    } else {
      // from.kind is float/double.
      if (!isNumeric(to)) {
        return CAST_ERROR;
      }
    }

    var fromRank = PRIMITIVE_RANK[from.kind],
        toRank = PRIMITIVE_RANK[to.kind],
        fromSigned = PRIMITIVE_SIGNED[from.kind],
        toSigned = PRIMITIVE_SIGNED[to.kind];
    if (fromRank > toRank) {
      return CAST_TRUNCATE;
    } else if (fromRank === toRank && fromSigned !== toSigned) {
      return CAST_SIGNED_UNSIGNED;
    }

    return isInteger(from) === isInteger(to) ?
        CAST_OK_PROMOTION :
        CAST_OK_CONVERSION;
  }

  function canCastPointer(from, to) {
    var fp = getPointerlikePointee(from),
        tp;
    if (isPointerlike(to)) {
      tp = getPointerlikePointee(to);
      if (fp.kind === VOID && tp.kind === VOID) {
        // Fall through to cv-check.
      } else if ((fp.kind === VOID && isFunction(tp)) ||
                 (isFunction(fp) && tp.kind === VOID)) {
        return CAST_FUNCTION_POINTER_VOID_POINTER;
      } else if (isFunction(fp) && isFunction(tp) && fp.kind !== tp.kind) {
        return isCompatibleWith(fp, tp) ?
            CAST_FUNCTION_POINTER_NOPROTO :
            CAST_INCOMPATIBLE_POINTERS;
      } else if (fp.kind === VOID || tp.kind === VOID) {
        // Strangely cv-checks are ignored when casting from/to void*.
        return CAST_OK_CONVERSION;
      } else if (!isCompatibleWith(fp.unqualified(), tp.unqualified())) {
        return CAST_INCOMPATIBLE_POINTERS;
      }

      if (!isLessOrEquallyQualified(fp.cv, tp.cv)) {
        return CAST_DISCARD_QUALIFIER;
      }

      return CAST_OK_EXACT;
    } else if (isInteger(to)) {
      return CAST_POINTER_TO_INT;
    } else {
      return CAST_ERROR;
    }
  }

  function isCompatibleWith(from, to) {
    from = getCanonical(from);
    to = getCanonical(to);

    if (isNumeric(from)) {
      return from.kind === to.kind &&
             from.cv === to.cv;
    }

    switch (from.kind) {
      case VOID:
        return from.kind === to.kind;
      case POINTER:
      case CONSTANTARRAY:
      case INCOMPLETEARRAY:
        if (!isPointerlike(to)) {
          return false;
        }

        return isCompatibleWith(getPointerlikePointee(from),
                                getPointerlikePointee(to)) &&
               from.cv === to.cv;
      case RECORD:
        return from.kind === to.kind &&
               from.tag === to.tag &&
               from.cv === to.cv &&
               from.isUnion === to.isUnion;
      case ENUM:
        return from.kind === to.kind &&
               from.tag === to.tag &&
               from.cv === to.cv;
      case FUNCTIONPROTO:
        return (from.kind === to.kind &&
                from.argTypes.length === to.argTypes.length &&
                isCompatibleWith(from.resultType, to.resultType) &&
                utils.everyArrayPair(from, to, isCompatibleWith)) ||
               (to.kind === FUNCTIONNOPROTO &&
                isCompatibleWith(from.resultType, to.resultType));
      case FUNCTIONNOPROTO:
        return (from.kind === to.kind || to.kind === FUNCTIONPROTO) &&
               isCompatibleWith(from.resultType, to.resultType);
      default:
        throw new Error('canCast: Unknown kind ' + from.kind);
    }
  }

  function getCastRank(from, to) {
    var result = canCast(from, to);
    if (isCastWarning(result) || isCastError(result)) {
      return -1;
    }
    return result;
  }

  function getFunctionCallRank(fnType, argTypes) {
    var result = [],
        castRank = null,
        i;

    for (i = 0; i < argTypes.length; ++i) {
      if (fnType.kind === FUNCTIONNOPROTO ||
          (fnType.variadic && i >= fnType.argTypes.length)) {
        castRank = CAST_OK_DEFAULT_PROMOTION;
      } else {
        castRank = getCastRank(argTypes[i], fnType.argTypes[i]);
        if (castRank < 0) {
          return null;
        }
      }

      result.push(castRank);
    }
    return result;
  }

  function compareFunctionCallRanks(fnRank1, fnRank2) {
    var result = 0,
        i;
    for (i = 0; i < fnRank1.length; ++i) {
      if (fnRank1[i] < fnRank2[i]) {
        if (result > 0) {
          return 0;
        }
        result = -1;
      } else if (fnRank1[i] > fnRank2[i]) {
        if (result < 0) {
          return 0;
        }
        result = 1;
      }
    }

    return result;
  }

  function getBestViableFunction(fnTypes, argTypes) {
    var bestFnIdx = -1,
        bestRank,
        isValid = false,
        i;
    for (i = 0; i < fnTypes.length; ++i) {
      var cmpResult,
          rank;
      if (!fnTypes[i].isViableForCall(argTypes)) {
        continue;
      }

      rank = getFunctionCallRank(fnTypes[i], argTypes);
      if (!rank) {
        continue;
      }

      if (bestFnIdx !== -1) {
        cmpResult = compareFunctionCallRanks(rank, bestRank);
        if (cmpResult === 0) {
          isValid = false;
          continue;
        } else if (cmpResult < 0) {
          continue;
        }
      }

      bestFnIdx = i;
      bestRank = rank;
      isValid = true;
    }

    return isValid ? bestFnIdx : -1;
  }


  return {
    // Types
    void: Void(),
    bool: Numeric(BOOL),
    char: Numeric(CHAR_S),
    uchar: Numeric(UCHAR),
    ushort: Numeric(USHORT),
    uint: Numeric(UINT),
    ulong: Numeric(ULONG),
    ulonglong: Numeric(ULONGLONG),
    schar: Numeric(SCHAR),
    wchar: Numeric(WCHAR),
    short: Numeric(SHORT),
    int: Numeric(INT),
    long: Numeric(LONG),
    longlong: Numeric(LONGLONG),
    float: Numeric(FLOAT),
    double: Numeric(DOUBLE),
    longdouble: Numeric(LONGDOUBLE),

    // Type constructors
    Void: Void,
    Numeric: Numeric,
    Pointer: Pointer,
    Record: Record,
    Enum: Enum,
    Typedef: Typedef,
    Function: FunctionProto,
    FunctionNoProto: FunctionNoProto,
    Array: ConstantArray,
    IncompleteArray: IncompleteArray,

    // Qualifiers
    CONST: IS_CONST,
    VOLATILE: IS_VOLATILE,
    RESTRICT: IS_RESTRICT,

    // Type constants
    VARIADIC: VARIADIC,
    NOT_VARIADIC: NOT_VARIADIC,
    STRUCT: STRUCT,
    UNION: UNION,

    // Kinds
    INVALID: INVALID,
    UNEXPOSED: UNEXPOSED,
    VOID: VOID,
    BOOL: BOOL,
    CHAR_U: CHAR_U,
    UCHAR: UCHAR,
    USHORT: USHORT,
    UINT: UINT,
    ULONG: ULONG,
    ULONGLONG: ULONGLONG,
    CHAR_S: CHAR_S,
    SCHAR: SCHAR,
    WCHAR: WCHAR,
    SHORT: SHORT,
    INT: INT,
    LONG: LONG,
    LONGLONG: LONGLONG,
    FLOAT: FLOAT,
    DOUBLE: DOUBLE,
    LONGDOUBLE: LONGDOUBLE,
    POINTER: POINTER,
    RECORD: RECORD,
    ENUM: ENUM,
    TYPEDEF: TYPEDEF,
    FUNCTIONPROTO: FUNCTIONPROTO,
    FUNCTIONNOPROTO: FUNCTIONNOPROTO,
    CONSTANTARRAY: CONSTANTARRAY,
    INCOMPLETEARRAY: INCOMPLETEARRAY,

    // Default char to signed
    CHAR: CHAR_S,

    // Cast results
    CAST_OK_EXACT: CAST_OK_EXACT,
    CAST_OK_PROMOTION: CAST_OK_PROMOTION,
    CAST_OK_CONVERSION: CAST_OK_CONVERSION,
    CAST_TRUNCATE: CAST_TRUNCATE,
    CAST_SIGNED_UNSIGNED: CAST_SIGNED_UNSIGNED,
    CAST_INT_TO_POINTER: CAST_INT_TO_POINTER,
    CAST_POINTER_TO_INT: CAST_POINTER_TO_INT,
    CAST_DISCARD_QUALIFIER: CAST_DISCARD_QUALIFIER,
    CAST_INT_TO_ENUM: CAST_INT_TO_ENUM,
    CAST_DIFFERENT_ENUMS: CAST_DIFFERENT_ENUMS,
    CAST_INCOMPATIBLE_POINTERS: CAST_INCOMPATIBLE_POINTERS,
    CAST_FUNCTION_POINTER_VOID_POINTER: CAST_FUNCTION_POINTER_VOID_POINTER,
    CAST_FUNCTION_POINTER_NOPROTO: CAST_FUNCTION_POINTER_NOPROTO,
    CAST_ERROR: CAST_ERROR,

    CALL_ERROR : CALL_ERROR,
    CALL_OK: CALL_OK,
    CALL_WARNING: CALL_WARNING,

    // Functions
    checkType: checkType,
    describeQualifier: describeQualifier,
    getBestViableFunction: getBestViableFunction,
    getCanonical: getCanonical,
    getSpelling: getSpelling,
    isCastError: isCastError,
    isCastOK: isCastOK,
    isCastWarning: isCastWarning,
    isLessOrEquallyQualified: isLessOrEquallyQualified,
    isLessQualified: isLessQualified,
    isMoreOrEquallyQualified: isMoreOrEquallyQualified,
    isMoreQualified: isMoreQualified,
  };

})(utils);

// mod /////////////////////////////////////////////////////////////////////////
var mod = (function(Long, type, utils) {

  var ERROR_IF_ID = -1;

  function numberToType(n) {
    if (!(isFinite(n) && (utils.isInteger(n) || utils.isUnsignedInteger(n)))) {
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
      } else if (n <= utils.S32_MAX) {
        return type.int;
      } else {
        return type.uint;
      }
    }
  }

  function longToType(l) {
    var bits = l.getNumBitsAbs();
    if (bits > 32) {
      if (l.isNegative() || bits < 64) {
        return type.longlong;
      } else {
        return type.ulonglong;
      }
    } else {
      return numberToType(l.toNumber());
    }
  }

  function objectToType(obj) {
    if (obj instanceof Long) {
      return longToType(obj);
    }

    var klass = utils.getClass(obj);
    switch (klass) {
      case 'Null':
        return type.Pointer(type.void);
      case 'Boolean':
        return type.schar;
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

  function argToHandle(context, arg) {
    return (arg instanceof Handle) ? arg: objectToHandle(context, arg);
  }

  function argsToHandles(context, args) {
    return Array.prototype.map.call(args, function(arg) {
      return argToHandle(context, arg);
    });
  }

  function handlesToIds(handles) {
    return Array.prototype.map.call(handles, function(h) { return h.id; });
  }

  function Module(embed) {
    if (!(this instanceof Module)) { return new Module(embed); }
    this.$embed_ = embed || null;
    this.$handles_ = new HandleList();
    this.$errors_ = {};
    this.$functions_ = {};
    this.$context = this.$createContext();
    this.$types = {};
    this.$tags = {};
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
          s,
          i,
          fn,
          retHandle;

      if (bestFnIdx < 0) {
        s = 'Call to "' + name + '" failed.\n';
        s += 'Got:\n  ' +
             name + '(' + argTypes.map(type.getSpelling).join(', ') + ')';
        s += '\nBut expected:\n';
        for (i = 0; i < fnTypes.length; ++i) {
          s += '  ' + type.getSpelling(fnTypes[i], name);
        }

        throw new Error(s);
      }

      fn = functions[bestFnIdx];
      if (fn.type.resultType !== type.void) {
        retHandle = self.$context.createHandle(fn.type.resultType);
      }

      self.$registerHandlesWithValues_(argHandles);
      self.$pushCommand_(fn.id, argHandles, retHandle);

      return retHandle;
    };

    this[name].types = fnTypes;
  };
  Object.defineProperty(Module.prototype, '$functionsCount', {
    get: function() { return Object.keys(this.$functions_).length; }
  });
  Object.defineProperty(Module.prototype, '$typesCount', {
    get: function() { return Object.keys(this.$types).length; }
  });
  Object.defineProperty(Module.prototype, '$tagsCount', {
    get: function() { return Object.keys(this.$tags).length; }
  });
  Module.prototype.$createContext = function() {
    return new Context(this.$handles_);
  };
  Module.prototype.$initMessage_ = function() {
    this.$message_ = {
    };
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
    if (handle.value === undefined) {
      return;
    }

    var value = handle.value;

    if (value instanceof Long) {
      value = [value.getLowBits(), value.getHighBits()];
    }

    if (!this.$message_.set) {
      this.$message_.set = {};
    }

    this.$message_.set[handle.id] = value;
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

    if (!this.$message_.commands) {
      this.$message_.commands = [];
    }

    this.$message_.commands.push(command);

    // Return the index of the last added command.
    return this.$message_.commands.length - 1;
  };
  Module.prototype.$processValues_ = function(handles, values) {
    var i;

    for (i = 0; i < handles.length; ++i) {
      if (handles[i].type.kind === type.LONGLONG ||
          handles[i].type.kind === type.ULONGLONG) {
        if (utils.getClass(values[i]) !== 'Array') {
          throw new Error('Expected longlong to have Array value type, not ' +
                          utils.getClass(values[i]));
        }

        if (values[i].length !== 2) {
          throw new Error('Expected longlong value to be Array of length 2, ' +
                          'not ' + values[i].length);
        }
        values[i] = Long(values[i][0], values[i][1]);
      }
    }

    return values;
  };
  Module.prototype.$commit = function(handles, callback) {
    var self = this,
        context = this.$context;

    if (callback.length !== handles.length &&
        callback.length !== handles.length + 1) {
      throw new Error('Expected callback to have ' + handles.length + ' or ' +
                      handles.length + 1 + ' arguments.');
    }

    if (!this.$message_.get) {
      this.$message_.get = [];
    }

    this.$message_.get = handlesToIds(handles);
    this.$embed_.postMessage(this.$message_, function(msg) {
      // Call the callback with the same context as was set when $commit() was
      // called, then reset to the previous value.
      var oldContext = self.$context,
          values = self.$processValues_(handles, msg.values),
          expectedError = callback.length === handles.length + 1,
          error;

      self.$context = context;
      if (typeof msg.error !== 'undefined') {
        error = self.$getError_(msg.error);

        if (expectedError) {
          values.unshift(error);
        } else {
          // Nowhere to pass the error. Just log it.
          console.error('Command at index ' + error.failedAt + ' failed:\n' +
                        error.stack);
        }
      } else if (expectedError) {
        values.unshift(undefined);
      }
      self.$clearErrors_();
      callback.apply(null, values);
      self.$context = oldContext;
    });
    this.$initMessage_();
  };
  Module.prototype.$destroyHandles = function(context) {
    var c = context || this.$context,
        handles = c.handles,
        i;

    if (!this.$message_.destroy) {
      this.$message_.destroy = [];
    }

    for (i = 0; i < handles.length; ++i) {
      this.$message_.destroy.push(handles[i].id);
    }

    c.destroyHandles();
  };
  Module.prototype.$commitDestroy = function(handles, callback) {
    this.$destroyHandles();
    this.$commit(handles, callback);
  };
  Module.prototype.$errorIf = function(arg) {
    var handle = argToHandle(this.$context, arg),
        hType = handle.type,
        commandIdx;

    if (hType.canCastTo(type.int) === type.CAST_ERROR) {
      throw new Error('$errorIf failed, invalid type: ' + hType.spelling);
    }

    this.$registerHandleWithValue_(handle);
    commandIdx = this.$pushCommand_(ERROR_IF_ID, [handle]);
    this.$registerError_(commandIdx, (new Error()).stack);
  };
  Module.prototype.$registerError_ = function(commandIdx, stack) {
    this.$errors_[commandIdx] = {
      failedAt: commandIdx,
      stack: stack
    };
  };
  Module.prototype.$getError_ = function(commandIdx) {
    return this.$errors_[commandIdx];
  };
  Module.prototype.$clearErrors_ = function() {
    this.$errors_ = {};
  };

  function IdFunction(id, fnType) {
    if (!(this instanceof IdFunction)) { return new IdFunction(id, fnType); }
    utils.checkNonnegativeNumber(id);
    type.checkType(fnType, 'type', [type.FUNCTIONPROTO, type.FUNCTIONNOPROTO]);

    if (id < 0) {
      throw new Error('Illegal id, reserved for built-in functions: ' + id);
    }

    this.id = id;
    this.type = fnType;
  }

  function HandleList() {
    this.nextId_ = 1;
    this.idToHandle_ = {};
  }
  HandleList.prototype.createHandle = function(context, type, value, id) {
    var register = false,
        handle;

    if (id === undefined) {
      id = this.nextId_++;
      register = true;
    }

    handle = new Handle(context, type, value, id);
    // Only register a handle if it was created without a given id (i.e. it was
    // not created as the result of a cast).
    if (register) {
      context.registerHandle(handle);
    }

    return handle;
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
  Context.prototype.destroyHandles = function() {
    // Call all finalizers. Run them in reverse order of the handle creation.
    var i,
        h;
    for (i = this.handles.length - 1; i >= 0; --i) {
      h = this.handles[i];
      if (h.finalizer) {
        h.finalizer(h);
      }
    }
    this.handles = [];
  };

  function Handle(context, type, value, id) {
    this.id = id;
    this.type = type;
    this.value = value;
    this.finalizer = null;
    this.context = context;
  }
  Handle.prototype.cast = function(toType) {
    var castResult = this.type.canCastTo(toType);
    if (castResult === type.CAST_ERROR) {
      throw new Error('Invalid cast: ' + this.type.spelling + ' to ' +
                      toType.spelling + '.');
    }

    return this.context.handleList.createHandle(
        this.context, toType, this.value, this.id);
  };
  Handle.prototype.setFinalizer = function(callback) {
    // Get the "root" handle, i.e. the one not created by casting.
    var root = this.context.handleList.get(this.id);
    if (root.finalizer) {
      throw new Error('Handle ' + root.id + ' already has finalizer.');
    }

    root.finalizer = callback.bind(root);
  };


  return {
    Module: Module,
    Function: IdFunction,

    numberToType: numberToType,
    longToType: longToType,
    objectToType: objectToType,

    ERROR_IF_ID: ERROR_IF_ID,
  };

})(Long, type, utils);

/** STRIP_START ***************************************************************/

  return {
    Embed: Embed,
    Long: Long,
    mod: mod,
    NaClEmbed: NaClEmbed,
    type: type,
    utils: utils,
  };
}));

/** STRIP_END *****************************************************************/
