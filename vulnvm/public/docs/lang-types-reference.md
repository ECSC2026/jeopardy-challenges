# Toybox.Lang Type Reference (condensed for implementation)

MonkeyC is dynamically/duck typed, reference counted, object oriented. Every value is
an Object; primitives have methods too. All types below ultimately derive from `Object`.

## Object (base of everything)
- `equals(other as Object or Null) as Boolean`
- `hashCode() as Number`
- `method(name as Symbol) as Method` — bound method reference
- `toString() as String`
- `weak() as WeakReference` — immutables return themselves

## Number (32-bit signed int)
- `abs() as Number`
- `compareTo(other as Object) as Number`
- `format(fmt as String) as String` — printf specifiers d i e E f o u x X
- `toChar() as Char`
- `toDouble() as Double`, `toFloat() as Float`, `toLong() as Long`, `toNumber() as Number`
- (inherits toString, hashCode, equals)

## Long (64-bit signed int; literal suffix `l`/`L`)
- `abs() as Long`
- `compareTo(other) as Number`
- `equals(other) as Boolean`
- `format(fmt as String) as String`
- `toDouble/toFloat/toLong/toNumber`, `toString() as String`

## Float (32-bit IEEE754)
- `abs() as Float`
- `compareTo(other) as Number` (NaN > all)
- `format(fmt as String) as String`
- `toDouble/toFloat/toLong/toNumber` (toNumber rounds toward zero)

## Double (64-bit IEEE754; literal suffix `d`/`D`)
- same shape as Float: abs, compareTo, format, toDouble/toFloat/toLong/toNumber, toString

## Boolean
- inherits Object methods (toString -> "true"/"false", equals, hashCode)

## Char (single unicode code point; literal `'c'`)
- `compareTo(other) as Number`
- `toLower() as Char`, `toUpper() as Char`
- `toNumber() as Number` (UTF-32 code point)
- `toString() as String`

## String
- `compareTo(other) as Number`
- `equals(other) as Boolean`
- `find(s as String) as Number or Null`
- `hashCode() as Number`
- `length() as Number`
- `substring(start as Number or Null, end as Number or Null) as String or Null`
- `toCharArray() as Array<Char>`
- `toDouble/toFloat/toLong/toNumber() ... or Null`
- `toLongWithBase(base as Number) as Long or Null`
- `toNumberWithBase(base as Number) as Number or Null`
- `toLower() as String`, `toUpper() as String`
- `toString() as String`
- `toUtf8Array() as Array<Number>`

## Array (dynamic, heterogeneous; literal `[a, b]`; `new [size]`)
- `add(obj) as Array`
- `addAll(array as Array) as Array`
- `indexOf(obj) as Number` (-1 if absent)
- `remove(obj) as Boolean`
- `removeAll(obj) as Boolean`
- `reverse() as Array`
- `size() as Number`
- `slice(start as Number or Null, end as Number or Null) as Array`
- `sort(comparator as Method or Null) as Void`
- `toString() as String`
- indexing: `a[i]` get/set

## ByteArray (array of bytes; `[]b` literal)
- `add(byte as Number|Char) as ByteArray`
- `addAll(array as Array|ByteArray) as ByteArray`
- `decodeNumber(format, options as Dictionary) as Numeric`
- `encodeNumber(value, format, options as Dictionary) as ByteArray`
- `equals(other) as Boolean`, `hashCode() as Number`
- `indexOf(byte) as Number`
- `remove(byte) as Boolean`, `removeAll(byte) as Boolean`
- `reverse() as ByteArray`
- `size() as Number`
- `slice(start, end) as ByteArray`
- `toString() as String`

## Dictionary (hash map; literal `{k => v}`)
- `get(key) as Object or Null`
- `put(key, value) as Void`
- `hasKey(key) as Boolean`
- `keys() as Array`
- `values() as Array`
- `remove(key) as Void`
- `size() as Number`
- `isEmpty() as Boolean`
- `toString() as String`
- indexing: `d[key]` get/set

## Symbol (`:name` literal; interned unique id)
- lightweight interned constant; used as dictionary keys / method names

## Method (bound method reference from `obj.method(:sym)` or `new Method`)
- `initialize(aClass, aMethod as Symbol)`
- `invoke(params...) as Object`
- `hashCode() as Number`

## WeakReference (from `obj.weak()`)
- `get() as Object or Null`
- `stillAlive() as Boolean`

## Exception (base) + subclasses
- `initialize()`
- `getErrorMessage() as String or Null`
- `printStackTrace() as Void`
- Subclasses: InvalidOptionsException, InvalidValueException,
  OperationNotAllowedException, SerializationException, StorageFullException,
  SymbolNotAllowedException, UnexpectedTypeException, ValueOutOfBoundsException

## Global functions in Lang
- `Lang.format(format as String, parameters as Array) as String` — `$1$`, `$2$` substitution
