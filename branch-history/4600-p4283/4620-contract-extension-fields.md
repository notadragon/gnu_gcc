---
id: 4620-contract-extension-fields
subject: 'c++: contracts: carry the new per-contract fields through construction and matching'
depends:
  - 1000-p2900-fixes
  - 3000-p3097
  - 3600-p3099
  - 4000-p3098
  - 4200-p3400-core
  - 4600-p4283
regenerates: []
fixes: [gcc-29]
---

## Rationale

Two functions have no single owner because each is a flat run over every
per-contract field the extension papers add, written as one indivisible hunk
that names four papers in a row.

`grok_contract` is the parse-time constructor of a contract node.  Its
signature gains three defaulted parameters at once -- the diagnostic message
(P3099), the assertion-control label (P3400) and the requires-clause
(P4283) -- it builds the node with its full operand count, stores the label and
the requires-clause, calls `resolve_contract_label`, calls
`finish_contract_message`, applies the `compute_comment` facet, and applies
[dcl.contract.func]/7 to a non-deferred postcondition predicate.  No one of
those is the change; the change is that all of them now happen here.

`mismatched_contracts_p` is the redeclaration-matching counterpart: four
consecutive comparison blocks, one per field -- message (P3099), label
(P3400), postcondition captures (P3098) and requires-clause (P4283) -- in a
single 125-line insertion with no unchanged line between them.

The two header declarations travel with them: `grok_contract`'s new signature,
and the block declaring `finish_contract_message` (P3099),
`maybe_define_contract_wrapper` (P3097) and `update_late_contract`'s new
`fndecl` parameter.

Placed after P4283, the last paper any of it depends on.

## Compile gap

None: every field, facet and helper this commit names is introduced by one of
its dependencies.  Before it, a contract parsed with a message, a label or a
requires-clause reaches `grok_contract` with nowhere to put it, and a
redeclaration differing only in one of those fields is accepted silently.

## Contents

- gcc/cp/contracts.cc : @grok_contract, @mismatched_contracts_p
- gcc/cp/contracts.h : @finish_contract_condition, @init_contracts
- gcc/cp/parser.cc : /grok_contract \(cont_assert,/, /Save pending capture data for late parsing \(P3098\)/, /Store captures on the postcondition node\./
- gcc/testsuite/g++.dg/contracts/cpp26/contract-assert-alt-spelling.C : *
