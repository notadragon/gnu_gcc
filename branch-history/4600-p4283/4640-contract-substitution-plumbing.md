---
id: 4640-contract-substitution-plumbing
subject: 'c++: contracts: P4283: substitute a contract from one agreed pattern declaration'
depends:
  - 1000-p2900-fixes
  - 4000-p3098
  - 4200-p3400-core
  - 4600-p4283
regenerates: []
fixes: [gcc-13, gcc-16, gcc-634, gcc-652, gcc-656]
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
instantiation's own proxy, an explicit pack capture's included.

A lambda's contracts are substituted while its `operator()' is being defined,
so `tsubst_contract_specifier' then uses that function's own `this' and its
own local specializations.  A stand-in `this' would make the predicate look
like a default member initializer, where nothing is captured, and a parameter
pack named only in a lambda's contract is captured only now that its elements
are known; a copy of the local specializations would hide that capture from
the body, which would capture again (GCC-634: a fold over a captured pack in
a lambda's precondition).

The per-paper half, all inside `tsubst_contract' and all in one 152-line
insertion with no unchanged line between them:

* `4000-p3098' -- the postcondition captures are instantiated, scalar and
  pack, BEFORE the predicate, their initializers resolving the function
  parameters `tsubst_contract_specifier' registered.

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

A specialization's contracts are substituted with the specialization as the
current function, as the predicate was parsed, and its injected `this` is
given that context so that it is not taken for a default member
initializer's: a variable a statement expression in the predicate declares
belongs to the specialization, even for a generic lambda's operator()
instantiated inside the function that uses it (GCC-652,
`stmt-expr-local-generic-lambda.C`), and a lambda in a function template's
predicate is made local to the specialization, so that it captures the
elements of a parameter pack (GCC-656, `predicate-lambda-pack-capture.C`).
A postcondition of a generic lambda with a deduced return type still ICEs
(GCC-660, watched by `open-bug-stmt-expr-local-generic-lambda-deduced-post.C`).

The P3098 captures are substituted outside the template mode that a
postcondition with a result name raises while the return type is undeduced:
a capture cannot name the result, and nothing substitutes it again after the
deduction, so an initializer such as `t.m` stayed a template tree (GCC-477,
`p3098-capture-deduced-return.C`).

## Compile gap

None: every field and helper it names is introduced by a dependency.  Before
it, a lambda's contracts are the pattern's, a templated label is never
resolved, and a requires-clause is parsed and then ignored.

## Contents

- gcc/cp/pt.cc : /A lambda in a function template's contract predicate is local to the$/, /^contract_requires_satisfied_p \(tree expr, tree args, tree in_decl\)$/, @bridge_lambda_capture_proxy_r, @regenerate_decl_from_template, @subst_contract_specifiers, @tsubst_contract_specifier, @tsubst_contract_specifiers, @tsubst_lambda_contract_specifiers, /DECL's parameters are already local specializations of IN_DECL's, so/, /contract_parameter_pattern \(tree in_decl\)/, /IN_DECL is the declaration whose parameters CONTRACT/, /tsubst_lambda_contract_specifiers \(fn, oldfn, args, complain\);/
- gcc/testsuite/g++.dg/contracts/cpp26/contract-outofline-member-late-definition.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-capture-contract-in-template.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-contract-in-template.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/lambda-pre-fold-capture.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/hidden-friend-template-pre.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/hidden-friend-template-pre-param.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/nsdmi-lambda-contract-this.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/stmt-expr-local-generic-lambda.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/predicate-lambda-pack-capture.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/open-bug-stmt-expr-local-generic-lambda-deduced-post.C : *
