# MonkeyC Language Notes (for lexer/compiler)

## Nature
- Object oriented, dynamically ("duck") typed. No true primitives — everything is an Object.
- Reference counted memory management.
- `as Type` annotations are optional and (for our VM) ignored at runtime.

## Keywords
Declaration: `var const function class module enum using import as native static`
Access:      `public private protected hidden`
Control:     `if else while for do return break continue switch case default`
Exceptions:  `try catch finally throw`
Operators/kw:`new self me null true false and or not instanceof has extends`

## Literals
- Number:  `42`, hex `0x1F`, (octal not required)
- Long:    `42l` / `42L`
- Float:   `3.14`, `1.0e3`
- Double:  `3.14d` / `3.14D`
- Char:    `'A'` (with escapes `\n \t \\ \' \0 \xNN`)
- String:  `"..."` (escapes; adjacent-concat not required)
- Boolean: `true` `false`
- Null:    `null`
- Symbol:  `:name`
- Array:   `[e0, e1, ...]`   sized: `new [n]`
- ByteArray: `[b0, b1]b`
- Dictionary: `{ k0 => v0, k1 => v1 }`

## Operators
- Arithmetic: `+ - * / %`  (and unary `-`)
- Comparison: `== != < > <= >=`
- Logical:    `&& || !`  (also keyword forms `and or not`)
- Bitwise:    `& | ^ ~ << >>`
- Assignment: `= += -= *= /= %= &= |= ^= <<= >>=`
- Increment:  `++ --` (sugar for `+= 1` / `-= 1`; statement-level only)
- Ternary:    `cond ? a : b`
- Membership: `x instanceof Type`, `obj has :symbol`
- Index/call/member: `a[i]`, `f(args)`, `obj.member`

## Comments
- Line: `// ...`
- Block: `/* ... */`  (doc `/** */` treated same)

## Declarations
- Variable:  `var x;` / `var x = expr;` (function/block scope)
- Constant:  `const K = expr;` (module/class scope only)
- Enum:      `enum { A, B, C = 5, D }` (auto-increment from 0)
- Function:  `function name(a, b) { ... }`  (default return null)
- Class:     `class Name extends Base { var f; function initialize() {...} function m() {...} }`
- Module:    `module Name { ... }` (namespace; no inheritance)
- Import:    `using Toybox.Lang;`  `using Toybox.Lang as L;`  `import ...;`

## Semantics notes
- `self` / `me` == `this`.
- `initialize()` is the constructor; `new Class(args)` allocates + calls it.
- Access modifiers apply only to class/module members; default is public.
- Statements are not expressions; assignment is not allowed inside an expression
  (no `x = (y = 3)`), and `if` conditions must be expressions, not assignments.
- Reference counting cleans up automatically; `weak()` avoids cycles.

