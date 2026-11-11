---
id: 0480-json-string-nul
subject: 'json: keep a string''s bytes after a \u0000 escape'
depends: []
regenerates: []
fixes: [gcc-987]
---

## Rationale

GCC-987.  Not a contracts bug.  The JSON lexer built a string token's
bytes, NUL included, into a C string with no length, and the parser made
the `json::string` from it with `strlen`, so a string holding a `\u0000`
escape was silently cut there: `"a\u0000b"` was `"a"`.

The token now records its string's length, and the parser passes it to
`json::string (const char *, size_t)`, so `get_length` and the bytes cover
the whole string.  Object keys are unchanged.  A selftest in
`test_parse_string` covers it.

## Compile gap

None.

## Contents

- gcc/json-parsing.cc : *
