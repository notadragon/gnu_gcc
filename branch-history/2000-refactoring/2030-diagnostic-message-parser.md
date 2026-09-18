---
id: 2030-diagnostic-message-parser
subject: 'c++: parser: factor the diagnostic-message grammar out of static_assert'
depends: []
regenerates: []
fixes: []
---

## Rationale

C++26 gives `static_assert' a *diagnostic-message*: either an
unevaluated-string or a constant-expression with `size()' and `data()'.  The
grammar production was written inline in `cp_parser_static_assert'.  P3099
gives a contract assertion the same production, so this commit lifts it into
`cp_parser_diagnostic_message' and calls it from `static_assert'.

Behaviour-neutral.  The lookahead that decides string-literal versus
expression, the `PAREN_EXPR' wrapper around a parenthesized `STRING_CST', and
the dialect-dependent literal parsing all move unchanged.  The one thing that
does not move is the pre-C++26 `-Wc++26-extensions' pedwarn: it is specific to
`static_assert', so the new function reports through an out-parameter and the
caller keeps the diagnostic.

The forward declarations travel with it because the net diff adds all three
new parser entry points in one hunk -- `cp_parser_diagnostic_message', which
this commit defines, plus `cp_parser_contract_message' (`3600-p3099') and
`cp_parser_contract_result_name' (`1000-p2900-fixes'), which it does not.
Declaring them here rather than there is what keeps `cp_parser_static_assert',
thousands of lines above the definition, able to see it at all.

## Compile gap

Two of the three forward declarations name functions that are not defined
until later commits.  Neither is called before then, so this is an unused
static declaration, not an undefined reference.

## Contents

- gcc/cp/parser.cc : @cp_parser_diagnostic_message, @cp_parser_late_contracts, @cp_parser_static_assert, /static tree cp_parser_diagnostic_message/
