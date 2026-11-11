---
id: 1380-await-in-predicate
subject: 'c++: contracts: P2900: reject an await- or yield-expression in a contract predicate'
depends: []
regenerates: []
fixes: []
---

## Rationale

GCC-643.  An await-expression or a yield-expression may appear in a
contract predicate only within a lambda in it ([expr.await]/2,
[expr.yield]).  GCC had no such rule: `contract_assert (co_await a)` was
rejected for an unrelated reason ("could not convert '({...})' from 'void'
to 'bool'"), and in a coroutine's precondition `co_await` was accepted and
then crashed the gimplifier.

`cp_parser_in_contract_predicate_p` walks the binding levels out to the
nearest contract scope, stopping at a function's (a lambda's body), a
class's or a namespace's, so a statement expression in the predicate is
part of it and a lambda in it is not.  `co_await` and `co_yield` are
diagnosed there, as they are in a default argument.

## Compile gap

None.

## Contents

- gcc/cp/parser.cc : /^cp_parser_in_contract_predicate_p \(\)$/, /^\t    \/\* \.\.\. nor in a contract predicate\.  \*\/$/, /^  \/\* Nor in a contract predicate\.  \*\/$/
- gcc/testsuite/g++.dg/contracts/cpp26/await-in-predicate.C : *
