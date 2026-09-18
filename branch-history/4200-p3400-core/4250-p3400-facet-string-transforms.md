---
id: 4250-p3400-facet-string-transforms
subject: 'c++: contracts: the compute_comment and compute_message label facets'
depends:
  - 4200-p3400-core
regenerates: []
fixes: []
---

## Rationale

`compute_comment_label` and `compute_message_label`: the two facets that let
a label rewrite the strings a violation carries -- the comment (the
stringified predicate) and the message (the user-supplied text).

They are one commit and not two because they are one implementation.
`apply_label_string_facet` takes the facet name as a parameter; both facets
have the identical shape -- `const char* f(const char*) const`, invoked on a
constexpr control object, constant-evaluated, and the resulting pointer
turned back into a `STRING_CST`.  Splitting them would duplicate every line
and leave two commits each of which is a one-string diff of the other.

`extract_string_from_const_char_ptr` is the awkward half: the facet returns
a `const char*`, and recovering the characters means walking back from the
folded address through the `ADDR_EXPR` to the underlying `STRING_CST`, with
an offset when the label returned a pointer into the middle of a literal
(which `p3400-facet-comment-append.C` does).

`reresolve_contract_label_facets` is here rather than in the core because it
cannot be written without `apply_label_string_facet`.  It is the
instantiation-time re-entry point: when a dependent label becomes concrete,
the cached derived state is discarded (groups, both semantic slots, the
dynamic flag) and the string facets are re-applied -- the string facets
specifically, because unlike everything else they are applied *eagerly*, at
parse time, and a template's parse-time application ran against a label that
did not yet have a value.

## Compile gap

`apply_label_string_facet` calls `call_label_method` and
`constant_facet_value`; `reresolve_contract_label_facets` names the
`CONTRACT_*` accessors and is declared in `cp/contracts.h`.  All are in
`4200-p3400-core`, as are the `grok_contract` call sites, so with the core
present this builds.

The trap is that both facets fail open, into a string that is merely
*unmodified* rather than absent.  With the framework present and these two
unwired, a label carrying `compute_message` compiles, runs, and reports the
original message; there is no shape of program in which the omission
produces an error, because "this label does not transform the comment" is
the normal case for almost every label.  The `-Wcontract-invalid-label-facet`
near-miss check in the core covers the neighbouring mistake -- a member
that was meant to be the facet but is inaccessible or non-const -- but it
cannot say anything about a facet the compiler was never taught to call.

## Contents

- gcc/cp/contracts.cc : @apply_label_string_facet, @extract_string_from_const_char_ptr, @reresolve_contract_label_facets
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-comment-append.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-comment.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3400-facet-message.C : *
