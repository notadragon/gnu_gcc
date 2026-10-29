# GCC-617: C -- a statement expression in a prototype's contract is rejected

**Kind:** deferred (rejects valid) -- deferred by the user 2026-09-27
(statement expressions in C contracts)
**Status:** Open -- found 2026-09-27 (fixing GCC-557)
**Affects:** `-fcontracts-p4299` in C; a GNU statement expression in the
`_Pre`/`_Post` predicate of a function declaration that is not a definition
**Workaround:** put the contract on the definition only, or use a helper
function in the predicate
**Clang:** accepts it

## Symptom

[`c-stmt-expr-in-prototype.c`](c-stmt-expr-in-prototype.c): "braced-group
within expression allowed only inside a function".  The same predicate on a
definition is accepted.

## Cause

Since DECISIONS.md K7, `c_check_prototype_contracts` (`gcc/c/c-parser.cc`)
parses a prototype's predicates, with the parameters in scope, to diagnose
and discard them.  That happens outside any function, where
`c_parser_postfix_expression` rejects a statement expression
(`!building_stmt_list_p ()`).  Fix direction: parse them in a context that
allows a statement expression (a temporary statement list and block scope)
without letting a declaration inside it become a file-scope one.

## Watch test

`gcc.dg/contracts/open-bug-c-stmt-expr-in-prototype.c` (xfail `dg-bogus`),
mirrored in the Clang fork by the prototype line of
`clang/test/Contracts/C/c-stmt-expr-in-contract.c`.
