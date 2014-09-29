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

var assert = require('assert'),
    long = require('../../src/js/long'),
    Long = long.Long;

describe('Long', function() {
  describe('fromInt', function() {
    it('should work for int-sized values', function() {
      assert.strictEqual(Long.fromInt(0).toInt(), 0);
      assert.strictEqual(Long.fromInt(100).toInt(), 100);
      assert.strictEqual(Long.fromInt(10000).toInt(), 10000);
      assert.strictEqual(Long.fromInt(1000000).toInt(), 1000000);
      assert.strictEqual(Long.fromInt(-100).toInt(), -100);
      assert.strictEqual(Long.fromInt(-10000).toInt(), -10000);
      assert.strictEqual(Long.fromInt(-1000000).toInt(), -1000000);
    });
  });

  describe('fromNumber', function() {
    it('should return zero for non-finite values', function() {
      assert.strictEqual(Long.fromNumber(Infinity).toInt(), 0);
      assert.strictEqual(Long.fromNumber(-Infinity).toInt(), 0);
      assert.strictEqual(Long.fromNumber(NaN).toInt(), 0);
    });

    it('should return MIN_VALUE for values <= -2**63', function() {
      assert.strictEqual(Long.fromNumber(-Math.pow(2, 64)), Long.MIN_VALUE);
    });

    it('should return MAX_VALUE for values >= 2**63 - 1', function() {
      assert.strictEqual(Long.fromNumber(Math.pow(2, 64)), Long.MAX_VALUE);
    });

    it('should otherwise return normal long value', function() {
      assert.strictEqual(Long.fromNumber(100).toInt(), 100);
      assert.strictEqual(Long.fromNumber(-100000).toInt(), -100000);
      assert.strictEqual(Long.fromNumber(1099511627776).toNumber(),
                         1099511627776);
    });
  });

  describe('fromBits', function() {
    it('should set low and high 32-bit values', function() {
      assert.strictEqual(Long.fromBits(0, 0).toInt(), 0);
      assert.strictEqual(Long.fromBits(100, 0).toInt(), 100);
      assert.strictEqual(Long.fromBits(-1000, 0).toInt(), -1000);
      assert.strictEqual(Long.fromBits(0, 1).toNumber(), 4294967296);
      assert.strictEqual(Long.fromBits(0, 256).toNumber(), 1099511627776);
      assert.strictEqual(Long.fromBits(0, -256).toNumber(), -1099511627776);
    });
  });

  describe('toInt', function() {
    it('should truncate values whose abs. value >= 2**32', function() {
      assert.strictEqual(Long.fromBits(0, 1).toInt(), 0);
      assert.strictEqual(Long.fromBits(0, 256).toInt(), 0);
      assert.strictEqual(Long.fromBits(0, -256).toInt(), 0);
    });
  });

  describe('toNumber', function() {
    it('should round trip for +-2**x, x in [0, 64)', function() {
      var pow, val;

      for (pow = 0; pow < 64; ++pow) {
        val = Math.pow(2, pow);
        assert.strictEqual(Long.fromNumber(val).toNumber(), val);
        assert.strictEqual(Long.fromNumber(-val).toNumber(), -val);
      }
    });

    it('should round trip for +-10**x, x in [0, 19)', function() {
      var pow, val;

      for (pow = 0; pow < 19; ++pow) {
        val = Math.pow(10, pow);
        assert.strictEqual(Long.fromNumber(val).toNumber(), val);
        assert.strictEqual(Long.fromNumber(-val).toNumber(), -val);
      }
    });
  });

  describe('toString', function() {
    it('should work for zero', function() {
      assert.strictEqual(Long.ZERO.toString(), '0');
    });

    it('should work for +-2**x, x in [0, 55)', function() {
      var pow, val;

      // 2**55 and greater are not accurate as Numbers, the Long values are
      // correct.
      for (pow = 0; pow < 55; ++pow) {
        val = Math.pow(2, pow);
        assert.strictEqual(Long.fromNumber(val).toString(), val.toString());
        assert.strictEqual(Long.fromNumber(-val).toString(), (-val).toString());
      }
    });

    it('should work for +-10**x, x in [0, 19)', function() {
      var pow, val;

      for (pow = 0; pow < 19; ++pow) {
        val = Math.pow(10, pow);
        assert.strictEqual(Long.fromNumber(val).toString(), val.toString());
        assert.strictEqual(Long.fromNumber(-val).toString(), (-val).toString());
      }
    });

    it('should work for optional radix', function() {
      var radix;

      for (radix = 2; radix < 36; ++radix) {
        assert.strictEqual(Long.fromInt(1234).toString(radix),
                           (1234).toString(radix));
      }

      assert.strictEqual(Long.fromBits(0xdeadc0de, 0xcabba6e).toString(16),
                         'cabba6edeadc0de');
    });
  });

  describe('equals', function() {
    it('should work', function() {
      assert.ok(Long.MIN_VALUE.equals(Long.MIN_VALUE));
      assert.ok(Long.MAX_VALUE.equals(Long.MAX_VALUE));
      assert.ok(Long.ZERO.equals(Long.ZERO));
      assert.ok(Long.NEG_ONE.equals(Long.NEG_ONE));
      assert.ok(Long.ONE.equals(Long.ONE));
      assert.ok(Long.fromNumber(1000).equals(Long.fromNumber(1000)));
      assert.ok(Long.fromNumber(-1000).equals(Long.fromNumber(-1000)));
      assert.ok(Long.fromBits(0, 256).equals(Long.fromBits(0, 256)));
    });
  });

  describe('notEquals', function() {
    it('should work', function() {
      assert.ok(Long.MIN_VALUE.notEquals(Long.MAX_VALUE));
      assert.ok(Long.MAX_VALUE.notEquals(Long.MIN_VALUE));
      assert.ok(Long.ZERO.notEquals(Long.ONE));
      assert.ok(Long.NEG_ONE.notEquals(Long.ZERO));
      assert.ok(Long.ONE.notEquals(Long.NEG_ONE));
      assert.ok(Long.fromNumber(1000).notEquals(Long.fromNumber(-1000)));
      assert.ok(Long.fromNumber(-1000).notEquals(Long.fromNumber(1000)));
      assert.ok(Long.fromBits(0, 256).notEquals(Long.fromBits(256, 0)));
    });
  });

  describe('lessThan', function() {
    it('should work', function() {
      assert.ok(Long.MIN_VALUE.lessThan(Long.MAX_VALUE));
      assert.ok(Long.ZERO.lessThan(Long.MAX_VALUE));
      assert.ok(Long.ZERO.lessThan(Long.ONE));
      assert.ok(Long.NEG_ONE.lessThan(Long.ZERO));
      assert.ok(Long.ONE.notEquals(Long.fromNumber(10)));
      assert.ok(Long.fromNumber(-10000).lessThan(Long.fromNumber(-1000)));
      assert.ok(Long.fromNumber(-1000).lessThan(Long.fromNumber(1000)));
      assert.ok(Long.fromBits(256, 0).lessThan(Long.fromBits(0, 256)));
    });
  });

  describe('lessThanOrEqual', function() {
    it('should work', function() {
      assert.ok(Long.MIN_VALUE.lessThanOrEqual(Long.MAX_VALUE));
      assert.ok(Long.ZERO.lessThanOrEqual(Long.MAX_VALUE));
      assert.ok(Long.ZERO.lessThanOrEqual(Long.ONE));
      assert.ok(Long.NEG_ONE.lessThanOrEqual(Long.ZERO));
      assert.ok(Long.ONE.notEquals(Long.fromNumber(10)));
      assert.ok(Long.fromNumber(-1000).lessThanOrEqual(Long.fromNumber(-1000)));
      assert.ok(Long.fromNumber(-1000).lessThanOrEqual(Long.fromNumber(1000)));
      assert.ok(Long.fromBits(256, 0).lessThanOrEqual(Long.fromBits(0, 256)));
    });
  });

  describe('greaterThan', function() {
    it('should work', function() {
      assert.ok(Long.MAX_VALUE.greaterThan(Long.MIN_VALUE));
      assert.ok(Long.MAX_VALUE.greaterThan(Long.ZERO));
      assert.ok(Long.ONE.greaterThan(Long.ZERO));
      assert.ok(Long.ZERO.greaterThan(Long.NEG_ONE));
      assert.ok(Long.fromNumber(10).notEquals(Long.ONE));
      assert.ok(Long.fromNumber(-1000).greaterThan(Long.fromNumber(-10000)));
      assert.ok(Long.fromNumber(1000).greaterThan(Long.fromNumber(-1000)));
      assert.ok(Long.fromBits(0, 256).greaterThan(Long.fromBits(256, 0)));
    });
  });

  describe('greaterThanOrEqual', function() {
    it('should work', function() {
      assert.ok(Long.MAX_VALUE.greaterThanOrEqual(Long.MIN_VALUE));
      assert.ok(Long.MAX_VALUE.greaterThanOrEqual(Long.ZERO));
      assert.ok(Long.ONE.greaterThanOrEqual(Long.ZERO));
      assert.ok(Long.ZERO.greaterThanOrEqual(Long.NEG_ONE));
      assert.ok(Long.fromNumber(10).notEquals(Long.ONE));
      assert.ok(Long.fromNumber(-1).greaterThanOrEqual(Long.fromNumber(-1000)));
      assert.ok(Long.fromNumber(1000).greaterThanOrEqual(Long.fromNumber(-10)));
      assert.ok(Long.fromBits(0, 25).greaterThanOrEqual(Long.fromBits(256, 0)));
    });
  });
});
