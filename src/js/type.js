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
    POINTER = 19,
    RECORD = 20,
    ENUM = 21,
    TYPEDEF = 22,
    FUNCTIONPROTO = 23,
    CONSTANTARRAY = 24,
    INCOMPLETEARRAY = 25,

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

    IS_CONST = 1,
    IS_VOLATILE = 2,
    IS_RESTRICT = 4,

    VARIADIC = true,
    NOT_VARIADIC = false,

    STRUCT = false,
    UNION = true,

    CAST_ERROR = 0,
    CAST_OK = 1,
    CAST_TRUNCATE = 2,
    CAST_SIGNED_UNSIGNED = 3,
    CAST_INT_TO_POINTER = 4,
    CAST_POINTER_TO_INT = 5,
    CAST_DISCARD_QUALIFIER = 6,
    CAST_INT_TO_ENUM = 7,
    CAST_DIFFERENT_ENUMS = 8,
    CAST_INCOMPATIBLE_POINTERS = 9,
    CAST_FUNCTION_POINTER_TO_VOID_POINTER = 10,
    CAST_VOID_POINTER_TO_FUNCTION_POINTER = 11,

    CALL_ERROR = 0,
    CALL_OK = 1,
    CALL_WARNING = 2,

    SPELLING_PRECEDENCE = {};

SPELLING_PRECEDENCE[POINTER] = 1;
SPELLING_PRECEDENCE[CONSTANTARRAY] = 2;
SPELLING_PRECEDENCE[INCOMPLETEARRAY] = 2;
SPELLING_PRECEDENCE[FUNCTIONPROTO] = 3;

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

function isVoid(type) {
  return type.kind === VOID;
}

function isInteger(type) {
  return type.kind >= BOOL && type.kind <= LONGLONG;
}

function isNumeric(type) {
  return type.kind >= BOOL && type.kind <= DOUBLE;
}

function isPointerlike(type) {
  return type.kind === POINTER || type.kind === CONSTANTARRAY ||
         type.kind === INCOMPLETEARRAY;
}

function isArray(type) {
  return type.kind === CONSTANTARRAY || type.kind === INCOMPLETEARRAY;
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
      return ConstantArray(recurse(type.elementType), type.arraySize, type.cv);
    case INCOMPLETEARRAY:
      return IncompleteArray(recurse(type.elementType), type.cv);
    case FUNCTIONPROTO:
      return FunctionProto(recurse(type.resultType),
                           Array.prototype.map.call(type.argTypes, recurse),
                           type.variadic);
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
Type.prototype.equals = function(that) {
  return this.kind === that.kind && this.cv === that.cv;
};

function Void(cv) {
  if (!(this instanceof Void)) { return new Void(cv); }
  Type.call(this, VOID, cv);
  this.spelling = getSpelling(this);
}
Void.prototype = Object.create(Type.prototype);
Void.prototype.constructor = Void;
Void.prototype.qualify = function(cv) {
  return Void(this.cv | cv);
};
Void.prototype.unqualified = function() {
  return Void();
};
Void.prototype.equals = function(that) {
  return Type.prototype.equals.call(this, that);
};

function Numeric(kind, cv) {
  if (!(this instanceof Numeric)) { return new Numeric(kind, cv); }
  Type.call(this, kind, cv);
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
Numeric.prototype.equals = function(that) {
  return Type.prototype.equals.call(this, that);
};

function Pointer(pointee, cv) {
  if (!(this instanceof Pointer)) { return new Pointer(pointee, cv); }
  Type.call(this, POINTER, cv);
  this.pointee = pointee;
  this.spelling = getSpelling(this);
}
Pointer.prototype = Object.create(Type.prototype);
Pointer.prototype.constructor = Pointer;
Pointer.prototype.qualify = function(cv) {
  return Pointer(this.pointee, this.cv | cv);
};
Pointer.prototype.unqualified = function() {
  return Pointer(this.pointee);
};
Pointer.prototype.equals = function(that) {
  return Type.prototype.equals.call(this, that) &&
         this.pointee.equals(that.pointee);
};

function Record(tag, fields, isUnion, cv) {
  if (!(this instanceof Record)) {
    return new Record(tag, fields, isUnion, cv);
  }

  if (tag !== null && getClass(tag) !== 'String') {
    throw new Error('Record tag must be null or string.');
  }

  if (!(fields instanceof Array)) {
    throw new Error('Record fields must be an Array.');
  }

  if (!fields.every(function(f) { return f instanceof Field; })) {
    throw new Error('Record fields must be of type Field.');
  }

  Type.call(this, RECORD, cv);
  this.tag = tag;
  this.fields = fields;
  this.isUnion = isUnion || false;
  this.spelling = getSpelling(this);
}
Record.prototype = Object.create(Type.prototype);
Record.prototype.constructor = Record;
Record.prototype.qualify = function(cv) {
  return Record(this.tag, this.fields, this.isUnion, this.cv | cv);
};
Record.prototype.unqualified = function() {
  return Record(this.tag, this.fields, this.isUnion);
};
Record.prototype.equals = function(that) {
  return Type.prototype.equals.call(this, that) &&
         this.tag === that.tag &&
         everyArrayPair(this.fields, that.fields,
             function(f1, f2) { return f1.equals(f2); }) &&
         this.isUnion === that.isUnion;
};

function Field(name, type, offset) {
  if (!(this instanceof Field)) { return new Field(name, type, offset); }
  this.name = name;
  this.type = type;
  this.offset = offset;
}
Field.prototype.equals = function(that) {
  return this.name === that.name &&
         this.type.equals(that.type) &&
         this.offset === that.offset;
};


function Enum(tag, cv) {
  if (!(this instanceof Enum)) { return new Enum(tag, cv); }
  Type.call(this, ENUM, cv);
  this.tag = tag;
  this.spelling = getSpelling(this);
}
Enum.prototype = Object.create(Type.prototype);
Enum.prototype.constructor = Enum;
Enum.prototype.qualify = function(cv) {
  return Enum(this.tag, this.cv | cv);
};
Enum.prototype.unqualified = function() {
  return Enum(this.tag);
};
Enum.prototype.equals = function(that) {
  return Type.prototype.equals.call(this, that) &&
         this.tag === that.tag;
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
Typedef.prototype.equals = function(that) {
  return Type.prototype.equals.call(this, that) &&
         this.tag === that.tag &&
         this.alias.equals(that.alias);
};

function FunctionProto(resultType, argTypes, variadic) {
  if (!(this instanceof FunctionProto)) {
    return new FunctionProto(resultType, argTypes, variadic);
  }

  if (isArray(getCanonical(resultType))) {
    throw new Error('Function return type cannot be an array. Got ' +
                    resultType.spelling);
  }

  if (!(argTypes instanceof Array)) {
    throw new Error('argTypes must be an array.');
  }

  if (variadic && argTypes.length === 0) {
    throw new Error('Cannot create variadic function with no arguments.');
  }

  if (Array.prototype.some.call(argTypes, compose(getCanonical, isVoid))) {
    throw new Error('Function argument type cannot be void.');
  }

  Type.call(this, FUNCTIONPROTO, 0);
  this.resultType = resultType;
  this.argTypes = argTypes;
  this.variadic = variadic || false;
  this.spelling = getSpelling(this);
}
FunctionProto.prototype = Object.create(Type.prototype);
FunctionProto.prototype.constructor = FunctionProto;
FunctionProto.prototype.qualify = function(cv) {
  return this;
};
FunctionProto.prototype.unqualified = function() {
  return this;
};
FunctionProto.prototype.equals = function(that) {
  return Type.prototype.equals.call(this, that) &&
         this.resultType.equals(that.resultType) &&
         everyArrayPair(this.argTypes, that.argTypes, equals) &&
         this.variadic === that.variadic;
};
FunctionProto.prototype.canCallWith = function() {
  if (arguments.length !== this.argTypes.length) {
    return CALL_ERROR;
  }

  var i, result;
  for (i = 0; i < this.argTypes.length; ++i) {
    result = arguments[i].canCastTo(this.argTypes[i]);
    if (result === CAST_ERROR) {
      return CALL_ERROR;
    } else if (result !== CAST_OK) {
      return CALL_WARNING;
    }
  }

  return CALL_OK;
};

function ConstantArray(elementType, arraySize) {
  if (!(this instanceof ConstantArray)) {
    return new ConstantArray(elementType, arraySize);
  }

  if (elementType.kind === VOID) {
    throw new Error('Cannot create an array of voids.');
  }

  Type.call(this, CONSTANTARRAY, 0);
  this.elementType = elementType;
  this.arraySize = arraySize;
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
ConstantArray.prototype.equals = function(that) {
  return Type.prototype.equals.call(this, that) &&
         this.elementType.equals(that.elementType) &&
         this.arraySize === that.arraySize;
};

function IncompleteArray(elementType) {
  if (!(this instanceof IncompleteArray)) {
    return new IncompleteArray(elementType);
  }

  if (elementType.kind === VOID) {
    throw new Error('Cannot create an array of voids.');
  }

  Type.call(this, INCOMPLETEARRAY, 0);
  this.elementType = elementType;
  this.spelling = getSpelling(this);
}
IncompleteArray.prototype = Object.create(Type.prototype);
IncompleteArray.prototype.constructor = IncompleteArray;
IncompleteArray.prototype.qualify = function(cv) {
  return this;
};
IncompleteArray.prototype.unqualified = function() {
  return this;
};
IncompleteArray.prototype.equals = function(that) {
  return Type.prototype.equals.call(this, that) &&
         this.elementType.equals(that.elementType);
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
      argsSpelling = type.argTypes.map(function(a) { return getSpelling(a); });
      if (type.variadic) {
        argsSpelling.push('...');
      }
      name += argsSpelling.join(', ');
    } else {
      name += 'void';
    }
    name += ')';
    spelling = getSpelling(type.resultType, name, FUNCTIONPROTO);
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
      return to.kind === VOID;
    case POINTER:
    case CONSTANTARRAY:
    case INCOMPLETEARRAY:
      return canCastPointer(from, to);
    case RECORD:
      return from.kind === to.kind &&
             from.tag === to.tag &&
             from.isUnion === to.isUnion ?
          CAST_OK :
          CAST_ERROR;
    case ENUM:
      if (isInteger(to)) {
        return CAST_OK;
      } else if (to.kind === ENUM) {
        return from.tag === to.tag ? CAST_OK : CAST_DIFFERENT_ENUMS;
      } else {
        return CAST_ERROR;
      }
      break;
    case FUNCTIONPROTO:
      return CAST_ERROR;
    default:
      throw new Error('canCast: Unknown kind ' + from.kind);
  }
}

function canCastNumeric(from, to) {
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

  return CAST_OK;
}

function canCastPointer(from, to) {
  var fp = getPointerlikePointee(from),
      tp;
  if (isPointerlike(to)) {
    tp = getPointerlikePointee(to);
    if (fp.kind === VOID && tp.kind === VOID) {
      // Fall through to cv-check.
    } else if (fp.kind === VOID && tp.kind === FUNCTIONPROTO) {
      return CAST_VOID_POINTER_TO_FUNCTION_POINTER;
    } else if (fp.kind === FUNCTIONPROTO && tp.kind === VOID) {
      return CAST_FUNCTION_POINTER_TO_VOID_POINTER;
    } else if (fp.kind === VOID || tp.kind === VOID) {
      // Strangely cv-checks are ignored when casting from/to void*.
      return CAST_OK;
    } else if (!isCompatibleWith(fp.unqualified(), tp.unqualified())) {
      return CAST_INCOMPATIBLE_POINTERS;
    }

    if (!isLessOrEquallyQualified(fp.cv, tp.cv)) {
      return CAST_DISCARD_QUALIFIER;
    }

    return CAST_OK;
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
      return from.kind === to.kind &&
             from.argTypes.length === to.argTypes.length &&
             isCompatibleWith(from.resultType, to.resultType) &&
             everyArrayPair(from, to, isCompatibleWith);
    default:
      throw new Error('canCast: Unknown kind ' + from.kind);
  }
}

function equals(type1, type2) {
  return type1.equals(type2);
}


module.exports = {
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

  // Type constructors
  Void: Void,
  Numeric: Numeric,
  Pointer: Pointer,
  Record: Record,
  Field: Field,
  Enum: Enum,
  Typedef: Typedef,
  Function: FunctionProto,
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
  POINTER: POINTER,
  RECORD: RECORD,
  ENUM: ENUM,
  TYPEDEF: TYPEDEF,
  FUNCTIONPROTO: FUNCTIONPROTO,
  CONSTANTARRAY: CONSTANTARRAY,
  INCOMPLETEARRAY: INCOMPLETEARRAY,

  // Default char to signed
  CHAR: CHAR_S,

  // Cast results
  CAST_ERROR: CAST_ERROR,
  CAST_OK: CAST_OK,
  CAST_TRUNCATE: CAST_TRUNCATE,
  CAST_SIGNED_UNSIGNED: CAST_SIGNED_UNSIGNED,
  CAST_INT_TO_POINTER: CAST_INT_TO_POINTER,
  CAST_POINTER_TO_INT: CAST_POINTER_TO_INT,
  CAST_DISCARD_QUALIFIER: CAST_DISCARD_QUALIFIER,
  CAST_INT_TO_ENUM: CAST_INT_TO_ENUM,
  CAST_DIFFERENT_ENUMS: CAST_DIFFERENT_ENUMS,
  CAST_INCOMPATIBLE_POINTERS: CAST_INCOMPATIBLE_POINTERS,
  CAST_FUNCTION_POINTER_TO_VOID_POINTER: CAST_FUNCTION_POINTER_TO_VOID_POINTER,
  CAST_VOID_POINTER_TO_FUNCTION_POINTER: CAST_VOID_POINTER_TO_FUNCTION_POINTER,

  CALL_ERROR : CALL_ERROR,
  CALL_OK: CALL_OK,
  CALL_WARNING: CALL_WARNING,

  // Functions
  getSpelling: getSpelling,
  getCanonical: getCanonical,
  describeQualifier: describeQualifier,
  isLessQualified: isLessQualified,
  isMoreQualified: isMoreQualified,
  isLessOrEquallyQualified: isLessOrEquallyQualified,
  isMoreOrEquallyQualified: isMoreOrEquallyQualified,
};
