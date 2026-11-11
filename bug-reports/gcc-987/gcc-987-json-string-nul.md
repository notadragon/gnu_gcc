# GCC-987: the JSON parser cuts a string at a `\u0000` escape

**Status:** Fixed here
**Resolved by:** `0480-json-string-nul`
**Component:** other (`gcc/json-parsing.cc`)
**Keywords (ours -- upstream sets its own):** wrong-code (of the parsed value)
**Upstream Link:** None found (searched 2026-11-05).
**Affects:** read from the source -- `gcc/json-parsing.cc` on the stock
trunk mirror (`8802cd9fdbe`, 2026-10-04) and at this branch's base.  Not a
contracts bug.

## Reproducer

A selftest, since no stock `g++` option on x86_64 reads JSON (the parser's
stock users are `sarif-replay` and the aarch64 `-muser-provided-CPU=`
tunings).  In `test_parse_string`:

    parser_testcase tc ("\"a\\u0000b\"");
    const json::string *str = as_a <const json::string *> (tc.get_value ());
    ASSERT_EQ (str->get_length (), 3);   /* stock: 1 */

On stock the string is `"a"`, length 1: everything from the escaped NUL
on is lost, with no diagnostic.

## Cause

`lexer::lex_string` builds the token's UTF-8 bytes, NUL included, into a
`char *` (`token::u.string`) and records no length; `parser::parse_value`
then makes the `json::string` with the one-argument constructor, which
takes `strlen`.  `json::string` itself keeps a length (`get_length`), and
`json::string (const char *, size_t)` exists for exactly this.

## Fix (this branch)

The token records the string's length (`token::string_len`), and
`parse_value` passes it to `json::string (const char *, size_t)`.  Object
keys are still C strings.  A selftest in `test_parse_string` covers it.
This branch's contract configuration needs it to reject a `"group"` holding
a NUL, which it could not otherwise see.
