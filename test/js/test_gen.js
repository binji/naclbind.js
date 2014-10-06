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

var chai = require('chai'),
    assert = chai.assert,
    gen = require('naclbind-gen'),
    path = require('path');

chai.config.includeStack = true;

function appendPath(s, path) {
  return s.split(':').concat([path]).join(':');
}

function assertTypesEqual(t1, t2) {
  assert.ok(t1.equals(t2), 'types aren\'t equal: ' +
                           t1.spelling + ' != ' + t2.spelling);
}

function assertFieldsEqual(f, name, type, offset) {
  assert.strictEqual(name, f.name);
  assertTypesEqual(type, f.type);
  assert.strictEqual(offset, f.offset);
}

function genFile(infile, callback) {
  var basename = path.basename(infile);
      outdir = path.resolve(__dirname, '../../out/test/js', basename),
      outfile = path.join(outdir, 'gen.js'),
      inpath = path.join(__dirname, infile),
      opts = {
        template: 'glue.js'
      };

  gen.file(inpath, outfile, opts, function(error, outfile) {
    if (error) {
      return callback(error);
    }

    var glue = require(outfile),
        mod = glue.create();

    callback(null, mod, glue.type);
  });
}

describe('Generate JS', function() {
  this.slow(800);

  before(function() {
    if (!process.env.NACL_SDK_ROOT) {
      assert.ok(false, 'NACL_SDK_ROOT not set.');
    }
  });

  it('should do work for a simple function', function(done) {
    genFile('data/simple.h', function(error, m, type) {
      if (error) {
        assert.ok(false, 'Error generating JS.\n' + error);
      }

      var expected = type.Function(type.void, []);

      assert.strictEqual(1, m.$functionsCount);
      assert.ok(m.foo);
      assert.strictEqual(1, m.foo.types.length);
      assert.ok(expected.equals(m.foo.types[0]));
      assert.strictEqual(0, m.$typesCount);
      assert.strictEqual(0, m.$tagsCount);
      done();
    });
  });

  it('should generate struct types', function(done) {
    genFile('data/structs.h', function(error, m, type) {
      if (error) {
        assert.ok(false, 'Error generating JS.\n' + error);
      }

      assert.strictEqual(8, m.$functionsCount);
      assert.strictEqual(1, m.$typesCount);  // t1
      assert.strictEqual(11, m.$tagsCount);  // s1-s8, ns1, ns2, anonymous

      // Incomplete struct == s1
      assert.ok(m.$tags.s1);
      assert.strictEqual('s1', m.$tags.s1.tag);
      assert.strictEqual(-2, m.$tags.s1.size);
      assert.strictEqual(0, m.$tags.s1.fields.length);
      assert.strictEqual(false, m.$tags.s1.isUnion);

      // Empty struct == s2
      assert.ok(m.$tags.s2);
      assert.strictEqual('s2', m.$tags.s2.tag);
      assert.strictEqual(0, m.$tags.s2.size);
      assert.strictEqual(0, m.$tags.s2.fields.length);
      assert.strictEqual(false, m.$tags.s2.isUnion);

      // Basic struct == s3
      assert.ok(m.$tags.s3);
      assert.strictEqual('s3', m.$tags.s3.tag);
      assert.strictEqual(4, m.$tags.s3.size);
      assert.strictEqual(1, m.$tags.s3.fields.length);
      assertFieldsEqual(m.$tags.s3.fields[0], 'f', type.int, 0);
      assert.strictEqual(false, m.$tags.s3.isUnion);

      // Struct with typedef == s4, t1
      assert.ok(m.$tags.s4);
      assert.strictEqual('s4', m.$tags.s4.tag);
      assert.strictEqual(4, m.$tags.s4.size);
      assert.strictEqual(1, m.$tags.s4.fields.length);
      assertFieldsEqual(m.$tags.s4.fields[0], 'g', type.int, 0);
      assert.strictEqual(false, m.$tags.s4.isUnion);

      assert.ok(m.$types.t1);
      assert.strictEqual('t1', m.$types.t1.tag);
      assertTypesEqual(m.$tags.s4, m.$types.t1.alias);
      assert.strictEqual('t1', m.$types.t1.spelling);
      assert.strictEqual();

      // Anonymous nested struct, named field == s5
      assert.ok(m.$tags.s5);
      assert.strictEqual('s5', m.$tags.s5.tag);
      assert.strictEqual(8, m.$tags.s5.size);
      assert.strictEqual(1, m.$tags.s5.fields.length);
      assert.strictEqual('nested', m.$tags.s5.fields[0].name);
      assert.strictEqual(0, m.$tags.s5.fields[0].offset);
      assert.strictEqual(false, m.$tags.s5.isUnion);

      assert.strictEqual(2, m.$tags.s5.fields[0].type.fields.length);
      assertFieldsEqual(m.$tags.s5.fields[0].type.fields[0], 'f', type.int, 0);
      assertFieldsEqual(m.$tags.s5.fields[0].type.fields[1], 'g', type.int, 4);

      // Anonymous nested struct, unnamed field == s6
      assert.ok(m.$tags.s6);
      assert.strictEqual('s6', m.$tags.s6.tag);
      assert.strictEqual(8, m.$tags.s6.size);
      assert.strictEqual(2, m.$tags.s6.fields.length);
      assertFieldsEqual(m.$tags.s6.fields[0], 'f', type.int, 0);
      assertFieldsEqual(m.$tags.s6.fields[1], 'g', type.int, 4);
      assert.strictEqual(false, m.$tags.s6.isUnion);

      // Named nested struct, named field == s7
      assert.ok(m.$tags.s7);
      assert.strictEqual('s7', m.$tags.s7.tag);
      assert.strictEqual(8, m.$tags.s7.size);
      assert.strictEqual(2, m.$tags.s7.fields.length);
      assertFieldsEqual(m.$tags.s7.fields[0], 'g', m.$tags.ns1, 0);
      assertFieldsEqual(m.$tags.s7.fields[1], 'h', type.int, 4);
      assert.strictEqual(false, m.$tags.s7.isUnion);

      assert.ok(m.$tags.ns1);
      assert.strictEqual('ns1', m.$tags.ns1.tag);
      assert.strictEqual(4, m.$tags.ns1.size);
      assert.strictEqual(1, m.$tags.ns1.fields.length);
      assertFieldsEqual(m.$tags.ns1.fields[0], 'f', type.int, 0);

      // Named nested struct, unnamed field == s8
      assert.ok(m.$tags.s8);
      assert.strictEqual('s8', m.$tags.s8.tag);
      assert.strictEqual(4, m.$tags.s8.size);
      assert.strictEqual(1, m.$tags.s8.fields.length);
      assertFieldsEqual(m.$tags.s8.fields[0], 'g', type.int, 0);
      assert.strictEqual(false, m.$tags.s8.isUnion);

      done();
    });
  });

  it('should generate union types', function(done) {
    genFile('data/unions.h', function(error, m, type) {
      if (error) {
        assert.ok(false, 'Error generating JS.\n' + error);
      }

      assert.strictEqual(8, m.$functionsCount);
      assert.strictEqual(1, m.$typesCount);  // t1
      assert.strictEqual(11, m.$tagsCount);  // s1-s8, ns1, ns2, anonymous

      // Incomplete union == s1
      assert.ok(m.$tags.s1);
      assert.strictEqual('s1', m.$tags.s1.tag);
      assert.strictEqual(-2, m.$tags.s1.size);
      assert.strictEqual(0, m.$tags.s1.fields.length);
      assert.strictEqual(true, m.$tags.s1.isUnion);

      // Empty union == s2
      assert.ok(m.$tags.s2);
      assert.strictEqual('s2', m.$tags.s2.tag);
      assert.strictEqual(0, m.$tags.s2.size);
      assert.strictEqual(0, m.$tags.s2.fields.length);
      assert.strictEqual(true, m.$tags.s2.isUnion);

      // Basic union == s3
      assert.ok(m.$tags.s3);
      assert.strictEqual('s3', m.$tags.s3.tag);
      assert.strictEqual(4, m.$tags.s3.size);
      assert.strictEqual(1, m.$tags.s3.fields.length);
      assertFieldsEqual(m.$tags.s3.fields[0], 'f', type.int, 0);
      assert.strictEqual(true, m.$tags.s3.isUnion);

      // Union with typedef == s4, t1
      assert.ok(m.$tags.s4);
      assert.strictEqual('s4', m.$tags.s4.tag);
      assert.strictEqual(4, m.$tags.s4.size);
      assert.strictEqual(1, m.$tags.s4.fields.length);
      assertFieldsEqual(m.$tags.s4.fields[0], 'g', type.int, 0);
      assert.strictEqual(true, m.$tags.s4.isUnion);

      assert.ok(m.$types.t1);
      assert.strictEqual('t1', m.$types.t1.tag);
      assertTypesEqual(m.$tags.s4, m.$types.t1.alias);
      assert.strictEqual('t1', m.$types.t1.spelling);
      assert.strictEqual();

      // Anonymous nested union, named field == s5
      assert.ok(m.$tags.s5);
      assert.strictEqual('s5', m.$tags.s5.tag);
      assert.strictEqual(4, m.$tags.s5.size);
      assert.strictEqual(1, m.$tags.s5.fields.length);
      assert.strictEqual('nested', m.$tags.s5.fields[0].name);
      assert.strictEqual(0, m.$tags.s5.fields[0].offset);
      assert.strictEqual(true, m.$tags.s5.isUnion);

      assert.strictEqual(2, m.$tags.s5.fields[0].type.fields.length);
      assertFieldsEqual(m.$tags.s5.fields[0].type.fields[0], 'f', type.int, 0);
      assertFieldsEqual(m.$tags.s5.fields[0].type.fields[1], 'g', type.int, 0);

      // Anonymous nested union, unnamed field == s6
      assert.ok(m.$tags.s6);
      assert.strictEqual('s6', m.$tags.s6.tag);
      assert.strictEqual(4, m.$tags.s6.size);
      assert.strictEqual(2, m.$tags.s6.fields.length);
      assertFieldsEqual(m.$tags.s6.fields[0], 'f', type.int, 0);
      assertFieldsEqual(m.$tags.s6.fields[1], 'g', type.int, 0);
      assert.strictEqual(true, m.$tags.s6.isUnion);

      // Named nested union, named field == s7
      assert.ok(m.$tags.s7);
      assert.strictEqual('s7', m.$tags.s7.tag);
      assert.strictEqual(4, m.$tags.s7.size);
      assert.strictEqual(2, m.$tags.s7.fields.length);
      assertFieldsEqual(m.$tags.s7.fields[0], 'g', m.$tags.ns1, 0);
      assertFieldsEqual(m.$tags.s7.fields[1], 'h', type.int, 0);
      assert.strictEqual(true, m.$tags.s7.isUnion);

      assert.ok(m.$tags.ns1);
      assert.strictEqual('ns1', m.$tags.ns1.tag);
      assert.strictEqual(4, m.$tags.ns1.size);
      assert.strictEqual(1, m.$tags.ns1.fields.length);
      assertFieldsEqual(m.$tags.ns1.fields[0], 'f', type.int, 0);

      // Named nested union, unnamed field == s8
      assert.ok(m.$tags.s8);
      assert.strictEqual('s8', m.$tags.s8.tag);
      assert.strictEqual(4, m.$tags.s8.size);
      assert.strictEqual(1, m.$tags.s8.fields.length);
      assertFieldsEqual(m.$tags.s8.fields[0], 'g', type.int, 0);
      assert.strictEqual(true, m.$tags.s8.isUnion);

      done();
    });
  });

  it('should generate primitive types', function(done) {
    genFile('data/primitive.h', function(error, m, type) {
      if (error) {
        assert.ok(false, 'Error generating JS.\n' + error);
      }

      assert.strictEqual(13, m.$functionsCount);
      assert.strictEqual(0, m.$typesCount);
      assert.strictEqual(0, m.$tagsCount);

      assert.strictEqual(m.f1.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.char]), m.f1.types[0]);

      assert.strictEqual(m.f2.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.schar]), m.f2.types[0]);

      assert.strictEqual(m.f3.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.uchar]), m.f3.types[0]);

      assert.strictEqual(m.f4.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.short]), m.f4.types[0]);

      assert.strictEqual(m.f5.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.ushort]), m.f5.types[0]);

      assert.strictEqual(m.f6.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.int]), m.f6.types[0]);

      assert.strictEqual(m.f7.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.uint]), m.f7.types[0]);

      assert.strictEqual(m.f8.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.long]), m.f8.types[0]);

      assert.strictEqual(m.f9.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.ulong]), m.f9.types[0]);

      assert.strictEqual(m.f10.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.longlong]),
                       m.f10.types[0]);

      assert.strictEqual(m.f11.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.ulonglong]),
                       m.f11.types[0]);

      assert.strictEqual(m.f12.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.float]), m.f12.types[0]);

      assert.strictEqual(m.f13.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.double]), m.f13.types[0]);

      done();
    })
  });

  it('should generate typedefs', function(done) {
    genFile('data/typedefs.h', function(error, m, type) {
      if (error) {
        assert.ok(false, 'Error generating JS.\n' + error);
      }

      var voidp = type.Pointer(type.void),
          charp = type.Pointer(type.char);

      assert.strictEqual(3, m.$functionsCount);
      assert.strictEqual(3, m.$typesCount);
      assert.strictEqual(0, m.$tagsCount);

      assert.ok(m.$types.t1);
      assert.strictEqual('t1', m.$types.t1.tag);
      assertTypesEqual(type.char, m.$types.t1.alias);

      assert.ok(m.$types.t2);
      assert.strictEqual('t2', m.$types.t2.tag);
      assertTypesEqual(voidp, m.$types.t2.alias);

      assert.ok(m.$types.t3);
      assert.strictEqual('t3', m.$types.t3.tag);
      assertTypesEqual(charp, m.$types.t3.alias);

      assert.ok(m.f1);
      assert.strictEqual(m.f1.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.char]), m.f1.types[0]);

      assert.ok(m.f2);
      assert.strictEqual(m.f2.types.length, 1);
      assertTypesEqual(type.Function(type.void, [voidp]), m.f2.types[0]);

      assert.ok(m.f3);
      assert.strictEqual(m.f3.types.length, 1);
      assertTypesEqual(type.Function(type.void, [charp]), m.f3.types[0]);

      done();
    })
  });

  it('should generate enums', function(done) {
    genFile('data/enums.h', function(error, m, type) {
      if (error) {
        assert.ok(false, 'Error generating JS.\n' + error);
      }

      var e1 = type.Enum('e1'),
          e2 = type.Enum('e2');

      assert.strictEqual(2, m.$functionsCount);
      assert.strictEqual(1, m.$typesCount);
      assert.strictEqual(2, m.$tagsCount);

      assert.ok(m.$tags.e1);
      assert.strictEqual('e1', m.$tags.e1.tag);

      assert.ok(m.$tags.e2);
      assert.strictEqual('e2', m.$tags.e2.tag);

      assert.ok(m.$types.t1);
      assert.strictEqual('t1', m.$types.t1.tag);
      assertTypesEqual(e2, m.$types.t1.alias);

      assert.ok(m.f1);
      assert.strictEqual(m.f1.types.length, 1);
      assertTypesEqual(type.Function(type.void, [e1]), m.f1.types[0]);

      assert.ok(m.f2);
      assert.strictEqual(m.f2.types.length, 1);
      assertTypesEqual(type.Function(type.void, [e2]), m.f2.types[0]);

      done();
    })
  });

  it('should generate functions with various attributes', function(done) {
    genFile('data/functions.h', function(error, m, type) {
      if (error) {
        assert.ok(false, 'Error generating JS.\n' + error);
      }

      var s1 = type.Record('s1', 0, type.STRUCT),
          s2 = type.Record('s2', 4, type.STRUCT);
          u1 = type.Record('u1', 0, type.UNION),
          voidp = type.Pointer(type.void),
          s1p = type.Pointer(s1),
          u1p = type.Pointer(u1),
          PFviE = type.Pointer(type.Function(type.void, [type.int])),
          // Array types decay to pointer types.
          intArr = type.Pointer(type.int),
          intArr10 = type.Pointer(type.int),
          argv = type.Pointer(type.Pointer(type.char));

      s2.addField('f', type.int, 0);

      assert.strictEqual(13, m.$functionsCount);
      assert.strictEqual(0, m.$typesCount);
      assert.strictEqual(3, m.$tagsCount);

      // Pointers
      assert.ok(m.f1);
      assert.strictEqual(m.f1.types.length, 1);
      assertTypesEqual(type.Function(type.void, [voidp]), m.f1.types[0]);

      assert.ok(m.f2);
      assert.strictEqual(m.f2.types.length, 1);
      assertTypesEqual(type.Function(type.void, [s1p]), m.f2.types[0]);

      assert.ok(m.f3);
      assert.strictEqual(m.f3.types.length, 1);
      assertTypesEqual(type.Function(type.void, [u1p]), m.f3.types[0]);

      // Multiple params
      assert.ok(m.f4);
      assert.strictEqual(m.f4.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.int, type.int, voidp]),
                       m.f4.types[0]);

      assert.ok(m.f5);
      assert.strictEqual(m.f5.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.int, s2]),
                       m.f5.types[0]);

      // Return values
      assert.ok(m.f6);
      assert.strictEqual(m.f6.types.length, 1);
      assertTypesEqual(type.Function(type.int, [type.int]), m.f6.types[0]);

      assert.ok(m.f7);
      assert.strictEqual(m.f7.types.length, 1);
      assertTypesEqual(type.Function(voidp, [type.uint]), m.f7.types[0]);

      assert.ok(m.f8);
      assert.strictEqual(m.f8.types.length, 1);
      assertTypesEqual(type.Function(s1, []), m.f8.types[0]);

      // Function pointers
      assert.ok(m.f9);
      assert.strictEqual(m.f9.types.length, 1);
      assertTypesEqual(type.Function(type.int, [PFviE]), m.f9.types[0]);

      assert.ok(m.f10);
      assert.strictEqual(m.f10.types.length, 1);
      assertTypesEqual(type.Function(PFviE, [type.int, PFviE]), m.f10.types[0]);

      // Arrays
      assert.ok(m.f11);
      assert.strictEqual(m.f11.types.length, 1);
      assertTypesEqual(type.Function(type.void, [intArr]), m.f11.types[0]);

      assert.ok(m.f12);
      assert.strictEqual(m.f12.types.length, 1);
      assertTypesEqual(type.Function(type.void, [intArr10]), m.f12.types[0]);

      assert.ok(m.f13);
      assert.strictEqual(m.f13.types.length, 1);
      assertTypesEqual(type.Function(type.void, [type.int, argv]),
                       m.f13.types[0]);

      done();
    })
  });

  it('should work with a function with no prototype', function(done) {
    genFile('data/noproto.h', function(error, m, type) {
      if (error) {
        assert.ok(false, 'Error generating JS.\n' + error);
      }

      assert.strictEqual(1, m.$functionsCount);
      assert.strictEqual(0, m.$typesCount);
      assert.strictEqual(0, m.$tagsCount);

      // Pointers
      assert.ok(m.foo);
      assert.strictEqual(m.foo.types.length, 1);
      assertTypesEqual(type.FunctionNoProto(type.int), m.foo.types[0]);

      done();
    });
  });

  it('should parse directives in comments', function(done) {
    genFile('data/directive.h', function(error, m, type) {
      if (error) {
        assert.ok(false, 'Error generating JS.\n' + error);
      }

      assert.strictEqual(1, m.$functionsCount);
      assert.strictEqual(0, m.$typesCount);
      assert.strictEqual(0, m.$tagsCount);

      // Pointers
      assert.ok(!m.dont_include_me);
      assert.ok(m.include_me);

      done();
    });
  });

  it('should handle self-referential record types', function(done) {
    genFile('data/self_reference.h', function(error, m, type) {
      if (error) {
        assert.ok(false, 'Error generating JS.\n' + error);
      }

      assert.strictEqual(1, m.$functionsCount);
      assert.strictEqual(0, m.$typesCount);
      assert.strictEqual(1, m.$tagsCount);

      assert.ok(m.$tags.List);
      assert.strictEqual('List', m.$tags.List.tag);
      assert.strictEqual(8, m.$tags.List.size);
      assert.strictEqual(2, m.$tags.List.fields.length);
      assert.strictEqual(false, m.$tags.List.isUnion);

      assertFieldsEqual(m.$tags.List.fields[0], 'item', type.int, 0);
      assertFieldsEqual(m.$tags.List.fields[1],
                        'next', type.Pointer(m.$tags.List), 4);

      done();
    });
  });

  it('should handle type dependencies', function(done) {
    genFile('data/deps.h', function(error, m, type) {
      if (error) {
        assert.ok(false, 'Error generating JS.\n' + error);
      }

      assert.strictEqual(2, m.$functionsCount);
      assert.strictEqual(1, m.$typesCount);
      assert.strictEqual(1, m.$tagsCount);

      assert.ok(m.$types.foo);
      assert.strictEqual('foo', m.$types.foo.tag);
      assertTypesEqual(type.int, m.$types.foo.alias);
      assert.strictEqual('foo', m.$types.foo.spelling);

      assert.ok(m.$tags.bar);
      assert.strictEqual('bar', m.$tags.bar.tag);
      assert.strictEqual(8, m.$tags.bar.size);
      assert.strictEqual(1, m.$tags.bar.fields.length);
      assert.strictEqual(false, m.$tags.bar.isUnion);

      assertFieldsEqual(m.$tags.bar.fields[0],
                        'stuff', type.Array(m.$types.foo, 2), 0);

      done();
    });

  });
});
