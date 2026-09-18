---
id: 4650-reentrancy-everything
subject: 'c++: contracts: re-entrant instantiation against every P3850 extension at once'
depends: [3000-p3097, 3600-p3099, 4000-p3098, 4200-p3400-core,
          4600-p4283, 4620-contract-extension-fields,
          4630-label-and-requires-parsing,
          4640-contract-substitution-plumbing]
regenerates: []
fixes: []
---

## Rationale

Re-entrant contract instantiation, exercised against every extension listed
in `depends` at once: P3400's label (`pre<lbl>`), P4283's requires-clause
(satisfied, unsatisfied-and-discarded, and combined with P3097 virtual
dispatch and with P3098 captures), P3099's message, P3097's virtual
dispatch, and P3098's capturing postcondition -- the last case
(`use_everything`) puts all five on one contract, on both ends of the
re-entrancy.  Like `contract-reentrancy-p3097-p3098.C` in
`4010-p3097-p3098-combined`, this belongs to none of the five papers
individually: it exists to pin that instantiating one contracted call from
inside another contract's still-substituting predicate does not corrupt
state shared across every extension GCC added, not just the two that alter
control flow.  `contract_assert`'s no-enclosing-scope case
(`use_body`/`f_body`) is the control: on-odr-use instantiation working when
it is *not* re-entered is what makes the re-entrant cases below it
meaningful rather than vacuous.

## Compile gap

Not "none".  Three pieces this test needs are outside its `depends` list
and, in sequence order, come *after* it:

* Every contract in this file that carries a label, a requires-clause or a
  message reaches `grok_contract` to be built -- but `grok_contract`'s
  three trailing parameters for exactly those fields (and the calls to
  `resolve_contract_label`, `finish_contract_message` and the
  requires-clause storage that use them) are not split across the papers
  that own each field; the whole function is one segment, claimed entirely
  by `4620-contract-extension-fields` (confirmed against the tool's own
  segment assignment, not just by reading the diff).  Before `4620`,
  `grok_contract` still has its four-parameter, pre-P3099/P3400/P4283
  signature.
* The label and requires-clause syntax has no parser entry point before
  `4630-label-and-requires-parsing`: `cp_parser_assertion_control_specifier`
  (from `4200-p3400-core`) and `cp_parser_contract_requires_clause` (from
  `4600-p4283`) are both defined but uncalled until `4630` adds the two call
  sites -- one hunk apiece, in `cp_parser_contract_assert` and
  `cp_parser_function_contract_specifier` -- that invoke them.
* Every contract in this file is on a template, and a label or
  requires-clause on a template is not resolved at parse time: `4640
  -contract-substitution-plumbing`'s 152-line insertion into
  `tsubst_contract` is what substitutes a label against a concrete argument
  and evaluates a requires-clause (via `contract_constraint_satisfied_p`,
  itself defined in `4600-p4283` but uncalled until `4640`) before deciding
  whether to keep or discard the contract.

So as sequenced, this entry's own `depends` -- `3000-p3097`, `3600-p3099`,
`4000-p3098`, `4200-p3400-core`, `4600-p4283` -- cover the five papers'
*existence* but not the shared plumbing (`4620`, `4630`, `4640`) that lets a
label, a message or a requires-clause actually reach a contract or survive
template substitution, and all three of those come later in sequence than
this entry.  There is no reading of this as already covered: the
segment-assignment check above is specific to the exact lines this test's
own source touches, not a general audit of `4620`/`4630`/`4640`'s wider
Contents, but for what the test needs, this is a real gap in what
`depends` declares rather than something a stub could paper over -- the
missing pieces are call sites and a struct field, not something with a
plausible no-op body.  Naming the three ids in `depends` would not close it,
since all three are sequenced after this entry; closing it means moving this
entry after `4640`, which is a change to the mapping's order rather than to
this document.  It is recorded here as a known gap instead.

## Contents

- gcc/testsuite/g++.dg/contracts/cpp26/contract-reentrancy-p3850.C : *
