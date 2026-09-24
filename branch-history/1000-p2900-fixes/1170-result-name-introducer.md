---
id: 1170-result-name-introducer
subject: 'c++: contracts: P2900: parse the result-name-introducer in one place'
depends: []
regenerates: []
fixes: [gcc-25]
---

## Rationale

The grammar is

    result-name-introducer:
        identifier attribute-specifier-seq[opt] :

and the parser recognised only the plain `identifier :` spelling, open-coded
inside `cp_parser_function_contract_specifier`.

GCC-25 (PR c++/125725) is the missing attribute-specifier-seq:
`post (r [[maybe_unused]]: c)` fell through and was parsed as the start of the
predicate, producing a cascade of five unrelated errors beginning with
`'r' was not declared in this scope`.  The same open-coding is why a result
name written on a `contract_assert`, where the grammar does not allow one, was
not diagnosed either but silently mis-parsed.

`cp_parser_contract_result_name` is the one parser for the production, and it
is called from both places -- the contract specifier, which passes the
attributes out, and `contract_assert`, which passes `postcondition_p` false so
the introducer is diagnosed and then skipped, leaving the rest of the
assertion intelligible.

Two things a reviewer should check.  The tentative parse: an attribute can
only follow the identifier, so when the token after the name is not a colon
the parser speculates, and if what follows the attributes is not a colon it
aborts and the whole thing goes back to being parsed as a predicate.  And the
recorded limitation: `result_attrs` is applied where the result variable is
built, which is only this path -- an in-class contract defers its predicate
and builds its result variable later from the identifier stored on the
contract node, which has nowhere to carry attributes.  An attribute on an
in-class postcondition result name is therefore accepted and ignored.  That is
tested rather than an oversight.

## Compile gap

None: the new function and both call sites are here.

## Contents

- gcc/cp/parser.cc : @cp_expr, /A result name is only meaningful on a postcondition/, /Check for postcondition identifiers\./, /RESULT_ATTRS holds any attribute-specifier-seq/, /if \(result_attrs && result != error_mark_node\)/
- gcc/testsuite/g++.dg/contracts/cpp26/pr125725.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/result-name-misplaced.C : *
