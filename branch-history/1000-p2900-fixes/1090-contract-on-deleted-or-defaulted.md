---
id: 1090-contract-on-deleted-or-defaulted
subject: 'c++: contracts: reject a contract on a deleted or first-defaulted function'
depends: []
regenerates: []
fixes: [gcc-23]
---

## Rationale

GCC-23 (PR c++/124486, PR c++/125403).  [dcl.contract.func]/6 forbids a
function-contract-specifier-seq on a deleted function or on a function
defaulted on its first declaration, alongside the virtual-function rule.
Upstream implemented only the virtual case; `dcl.contract.func.p6.C`
carried a `TODO` saying so.  A contract on `= delete` or on a
first-declaration `= default` was silently accepted and then never checked,
because such a function has no body to check it in.

`check_contract_on_defaulted_or_deleted` is the one new predicate, called from
the three places a declaration learns it is deleted or defaulted:
`cp_finish_decl` for a namespace-scope or out-of-class definition (two sites,
one per form) and `grokfield` for a member declared with `= delete` or
`= default` in the class body.

A reviewer should check the boundary the accepted case pins: a function
defaulted on a *later* declaration is fine, because its first declaration was
an ordinary one that the contract belongs to.  `pr124486-accepted.C` is that
case, and the five diagnosing siblings plus the standard-named test bound it
from the other side.

## Compile gap

None: one predicate, its `contracts.h` declaration, and three call sites, all
here.

`pr124486-accepted.C` names `-fcontracts-p3097` on its `dg-additional-options`
line, and that option is introduced by `3000-p3097`.  As sliced, that one file
does not run at this commit -- an unrecognized option fails the whole test --
while its five diagnosing siblings and `dcl.contract.func.p6.C` all do.  It is
kept here because all seven pin one predicate, and splitting the negative
cases from the positive one would leave the "must NOT reject" half
unreviewable next to the half it exists to bound.  The other way out is to
move that single file to `3000-p3097`.

## Contents

- gcc/cp/contracts.cc : @check_contract_on_defaulted_or_deleted
- gcc/cp/contracts.h : @check_redecl_contract
- gcc/cp/decl.cc : @cp_finish_decl
- gcc/cp/decl2.cc : @grokfield
- gcc/testsuite/g++.dg/contracts/cpp26/dcl.contract.func.p6.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/pr124486-accepted.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/pr124486-copy.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/pr124486-ctor.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/pr124486-deleted.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/pr124486-dtor.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/pr124486-post.C : *
