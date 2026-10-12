---
id: 1300-explicit-specialization-contracts
subject: "c++: contracts: P2900: carry an explicit specialization's contracts across its redeclarations"
depends: []
regenerates: []
fixes: [gcc-552]
---

## Rationale

GCC-552.  An explicit specialization is merged with an earlier declaration of
the same specialization inside check_explicit_specialization, by
register_specialization calling duplicate_decls -- but grokfndecl attached
the contracts only after that, so duplicate_decls never saw them: a
redeclaration with a different contract was not diagnosed, and a definition
that omitted the contract lost it (check_explicit_specialization then
cleared the contracts of every registered specialization, to keep the
template's from applying).

grokfndecl now attaches the contracts before the merge, and does not
overwrite a merged declaration's.  check_redecl_contract treats a merge with
an implicit instantiation as the specialization's first declaration: the
surviving node gets exactly the specialization's contracts, none if it
declares none -- which is what the blanket clearing in
check_explicit_specialization was for, so that goes.  And since an explicit
specialization is a function of its own, its redeclarations get the ordinary
checks: one that adds contracts its first declaration lacked is rejected
("declaration adds contracts"), as for any other function.

`contracts-tmpl-spec2.C` relied on the old leniency: its `g2<double>` added a
contract on redeclaration.  That case moves to
`contracts-tmpl-spec-redecl-adds.C`, now an error, and `g2<double>` declares
its contract up front.  The file is also made a run test: it was compile-only,
so its 141 `dg-output` lines were never checked, and they had gone stale (each
line number one past the contract it named).  The output matches them in
every other respect.

An explicit specialization of a member template of a class template
specialization (`template<> template<class Q> int A<long>::f (long a,
Q)`) is folded by `check_explicit_specialization` into the member template
the class specialization instantiated, not merged by `duplicate_decls`, so
`member_template_spec_contracts` does what the merge does for it: a first
declaration drops the instantiated member's contracts and gives the
parameters its own names, so that its predicate finds them; a redeclaration
is compared with the specialization's contracts (a changed or added
contract is diagnosed); and a definition without contracts keeps the
declaration's, rewritten in terms of its parameters
(`member-template-spec-contracts.C`).

## Compile gap

None.

## Contents

- gcc/cp/decl.cc : #6, #7, #8
- gcc/cp/contracts.cc : #115, #116, /^\/\* DECL declares an explicit specialization of a member template of a class$/, @member_template_spec_contracts
- gcc/cp/contracts.h : @make_postcondition_variable:2
- gcc/cp/pt.cc : #2, /^\t\tmember_template_spec_contracts$/, /^\t\tremove_fn_contract_specifiers \(result\);$/
- gcc/testsuite/g++.dg/contracts/cpp26/explicit-spec-contract-dropped.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/explicit-spec-contract-mismatch.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contracts-tmpl-spec2.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/contracts-tmpl-spec-redecl-adds.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/explicit-spec-decl-renamed-params.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/member-template-spec-contracts.C : *
