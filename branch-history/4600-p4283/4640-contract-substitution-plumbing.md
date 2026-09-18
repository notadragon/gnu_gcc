---
id: 4640-contract-substitution-plumbing
subject: 'c++: contracts: substitute a contract from one agreed pattern declaration'
depends:
  - 1000-p2900-fixes
  - 4000-p3098
  - 4200-p3400-core
  - 4600-p4283
regenerates: []
fixes: [gcc-13, gcc-16]
---

## Rationale

Template substitution of a contract is the one place on this branch where four
papers share a single function body, and the rework that lets them is itself
inseparable from them.

The structural half.  A contract is substituted against the parameters of the
declaration it was WRITTEN on, which for a non-specialization is the most
general template's -- and it has to be TAKEN from that same declaration, or the
predicate names parameters that were never registered.  Upstream, one caller
computed that declaration inline and another read the specifiers from a
different one.  `contract_parameter_pattern' names the rule once;
`subst_contract_specifiers' is the shared core the two entry points now call,
differing only in how they find the pattern and the arguments.  The second
entry point is new: a lambda's `operator()' never reaches
`regenerate_decl_from_template', so without
`tsubst_lambda_contract_specifiers' its contracts stayed the pattern's, and
`bridge_lambda_capture_proxy_r' maps a capture named in the predicate onto the
instantiation's own proxy.

The per-paper half, all inside `tsubst_contract' and all in one 152-line
insertion with no unchanged line between them:

* `4000-p3098' -- function parameters are registered in
  `local_specializations' (a parameter pack mapped to the whole argument pack,
  as `register_parameter_specializations' does) so that capture initializers
  can resolve them, and the postcondition captures are instantiated, scalar
  and pack, BEFORE the predicate.

* `4200-p3400-core' -- `grok_contract' only resolves a label that is not
  type-dependent, so for a templated contract nothing ran at parse time; the
  label is substituted here with `processing_template_decl' put back down, and
  `resolve_contract_label' / `reresolve_contract_label_facets' re-run on the
  concrete value.

* `4600-p4283' -- the requires-clause is substituted and tested FIRST, before
  the captures and the predicate, so a predicate that is valid only under the
  constraint is never instantiated when the constraint fails.  An unsatisfied
  contract substitutes to `NULL_TREE', which is why the substituted vector is
  now collected and then sized to the survivors.

Placed after P4283, the last paper it depends on.

## Compile gap

None: every field and helper it names is introduced by a dependency.  Before
it, a lambda's contracts are the pattern's, a templated label is never
resolved, and a requires-clause is parsed and then ignored.

## Contents

- gcc/cp/pt.cc : @bridge_lambda_capture_proxy_r, @regenerate_decl_from_template, @subst_contract_specifiers, @tsubst_contract_specifier, @tsubst_contract_specifiers, @tsubst_lambda_contract_specifiers, /Ensure function parameters are in local_specializations/, /contract_parameter_pattern \(tree in_decl\)/, /IN_DECL is the declaration whose parameters CONTRACT/, /tsubst_lambda_contract_specifiers \(fn, oldfn, args, complain\);/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-outofline-member-late-definition.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-capture-contract-in-template.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-contract-in-template.C : *
