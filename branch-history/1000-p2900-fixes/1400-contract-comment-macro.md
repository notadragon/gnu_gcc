---
id: 1400-contract-comment-macro
subject: 'c++, contracts: the comment of a predicate involving macros'
depends: []
regenerates: []
fixes: [gcc-707]
---

## Rationale

GCC-707.  The comment of a contract is meant to be the text of its
predicate, but it was read from the source between the spelling locations
of the predicate's first and last tokens.  A predicate that began or ended
inside a macro expansion therefore got the text from the macro's
definition to the contract, part of two `#define` lines, a replacement
list, or a predicate missing its macro name -- and with the two ends out
of order or in different files, the printed expression.

The comment is now formed by `get_source_text_of_tokens` from the
predicate's tokens and the location of the token after it, which says
where the contract was written.  Each token is lifted out of macro
expansions to that context, becoming the macro's name as invoked there.
Outside any expansion the comment is the source text from the first token
to the last, extended to the closing parenthesis of a function-like
invocation the last token comes from.  Inside a macro it is the text of
the replacement list, or of an argument as written, when every token is
from it.  Otherwise, and whenever macro locations are not tracked, it is
the predicate's tokens, separated where whitespace preceded a token or
two tokens were not written next to each other.

## Compile gap

The text reaches `grok_contract` and `update_late_contract` as a new last
parameter, `comment`, and the lines that declare it and pass it belong to
entries that rewrote those lines: `grok_contract`'s signature, its
declaration, the declaration of `update_late_contract` and the parser's two
calls of `grok_contract` to `4620-contract-extension-fields`, and
`update_late_contract`'s signature and its call in
`cp_parser_late_contract_condition` to `1060-postcondition-odr-use`.  The
C front end's use of `get_source_text_of_tokens` is in `5000-p4299`, with
the rest of that front end.

## Contents

- gcc/input.cc : /return get_source_text_between \(fc, expstart, expend\);/, /^\/\* Likewise, from the column of EXPSTART to the column of EXPEND, both$/, #5
- gcc/input.h : *
- gcc/cp/parser.cc : #49, #54, #65:1, #88, /cp_parser_contract_comment \(first, end/
- gcc/cp/contracts.cc : /^build_comment \(cp_expr condition, char \*str\)$/, #r1, /^      free \(comment\);$/, #170:0, #173:0
- gcc/testsuite/g++.dg/contracts/cpp26/contract-comment-macro.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-comment-macro.h : *
- gcc/testsuite/g++.dg/contracts/cpp26/contract-comment-macro-untracked.C : *
