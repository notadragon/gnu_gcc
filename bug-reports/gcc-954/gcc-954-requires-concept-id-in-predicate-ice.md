# GCC-954: A requires-expression whose parameter list starts with a concept-id, inside a function contract's predicate, aborts the compiler during error recovery

**Status:** Fixed here
**Resolved by:** none recorded -- not bisected; the branch's contract parser
diagnoses every shape below cleanly (see "This branch")
**Component:** c++ / contracts
**Keywords (ours -- upstream sets its own):** `ice-on-invalid-code`,
`error-recovery`
**Upstream Link:** None found (searched 2026-10-30) -- Bugzilla quicksearch
for `cp_parser_function_contract_specifier` returns PR127284, PR127572 and
PR127857 (different defects); `contract requires ice` and `pre requires
contract` return nothing related.

**Affects:** measured 2026-10-30 -- stock trunk `17.0.0 20261029`
(`6aae4093ee7`) aborts with an internal compiler error on both cases; `16.1.0`
and `16.2.0` abort on the same shapes, reported as `confused by earlier
errors, bailing out` (a release-checking build masks an ICE after an error
that way, and prints that line only on the ICE path).  Found while comparing
P4283-style requires-clauses on contract assertions across implementations.

## The bug

```c++
template <class T> concept C = true;
void f () pre requires (C<int>);                 // case A
void f () pre (requires (C<int> x) { x; });      // case B (-DCASE_B)
```

```
$ g++ -std=c++26 -fcontracts -fsyntax-only gcc-954-requires-concept-id-in-predicate-ice.cpp
...:22:15: error: expected '(' before 'requires'
...:22:25: error: wrong number of template arguments (2, should be 1)
...:22:32: error: expected '{' before ';' token
...: internal compiler error: in cp_parser_function_contract_specifier, at cp/parser.cc:34261
```

Case B, an ordinary parenthesized predicate holding an ill-formed
requires-expression, gives `wrong number of template arguments` and the same
internal compiler error.  Case A is how the abort was found: it is the P4283
spelling of a requires-clause on a contract assertion, which stock does not
support, so after `expected '('` the predicate parse reads `requires (C<int>)`
as a requires-expression.

What it takes:

| variation | trunk | 16.2 |
|---|---|---|
| `pre requires (C<int>)` | ice | ice (masked) |
| `pre (requires (C<int> x) { x; })` | ice | ice (masked) |
| `pre requires (b<int>)`, `b` a variable template | error | error |
| `pre requires (S<int>)`, `S` a class template | error | error |
| `pre requires (int)`, `pre requires ()` | error | error |
| `contract_assert (requires (C<int>))` | error | error |
| `bool b = requires (C<int>);` | error | error |
| `pre (requires (int x) { x; })` -- well-formed | clean | clean |

So: a function contract specifier (not `contract_assert`), a requires-expression
in its predicate, and a parameter list beginning with a concept-id.  Valid
code is not affected.

## Where the defect is

The assertion that fires is the scope check after the predicate is parsed in
`cp_parser_function_contract_specifier` (`gcc/cp/parser.cc`, at
`6aae4093ee7`):

```c++
      cp_expr condition = cp_parser_conditional_expression (parser);
      ...
      gcc_checking_assert (scope_chain && scope_chain->bindings
                           && scope_chain->bindings->kind == sk_contract);
      pop_bindings_and_leave_scope ();
```

The predicate parse returns with some other scope innermost.  The concept-id
at the start of the requires-expression's parameter list is parsed as a
type-constraint, and the failed parse of that parameter list (or of the
missing requirement body) appears not to leave the scope it entered before
returning to the contract parser.  `contract_assert` is not affected because
its statement-level path has no such assertion.

## This branch

`contracts-p3850-working` diagnoses both cases without aborting: case A as
`requires clause on contract assertions requires '-fcontracts-p4283'` (the
branch parses the P4283 form), case B with the same `wrong number of template
arguments` and no internal compiler error.  Not bisected to a commit; the
branch's contract parser differs from stock's in several places along this
path.
