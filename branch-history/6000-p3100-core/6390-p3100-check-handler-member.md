---
id: 6390-p3100-check-handler-member
subject: 'c++: contracts: P3100: implicit function-try-block handler member check'
depends:
  - 6000-p3100-core
regenerates: []
fixes: []
---

## Rationale

The implicit contract assertion `ub:except.handle.handler.ctor.dtor`:
referring to a non-static member or base of an object in a
function-try-block handler of its constructor or destructor, which
[except.handle] makes undefined -- by then every member and base has been
destroyed or was never initialized.

The check is lexical and unconditional: `build_implicit_handler_member_check`
prepends a violation to a member access (`build_class_member_access_expr`,
for a non-static data member, of a base too) or a member function call
whose object is `*this` written directly -- by name, as an operator or as a
conversion function, so in `build_over_call`, through
`p3100_member_call_checks`, also on the early returns for a trivial copy
assignment and a trivial destructor -- while
`in_function_try_handler` is set in a constructor or destructor.  A
lambda's captured `this`, a static member and another object of the class
are not checked, nor is a bare conversion of `this` to a base.

`build_implicit_unconditional_check` builds the violation as a guard whose
condition is a constant true, marked `COND_EXPR_IMPLICIT_UB_RUNTIME_P`, so
constant evaluation skips it and diagnoses the reference as before.
Nothing defined could replace the reference, so "ignore" emits nothing and
"observe" goes on with it.  The check reports the group itself: D4277R1
splits it by whether the member was ever initialized and whether it is
accessed, which a lexical check cannot tell.

## Compile gap

None: `resolve_implicit_contract_semantic`, `build_implicit_violation_call`
and `COND_EXPR_IMPLICIT_UB_RUNTIME_P` are in `6000-p3100-core`.

`in_function_try_handler` lives in `cp_function_chain`, which is `cfun`'s
language data: the check tests `cfun` first, since member accesses are
also built with no function at all (a contract label's facets, a deferred
contract predicate).

## Contents

- gcc/cp/typeck.cc : /tree p3100_object = object;/, /build_implicit_handler_member_check/
- gcc/cp/call.cc : /P3100: the object argument of a member function call/, /p3100_member_call_checks \(p3100_object, p3100_virtual, (call|val)\)/, #7:0, #7:2
- gcc/cp/contracts.cc : @implicit_this_object_p, @build_implicit_unconditional_check, @build_implicit_handler_member_check
- gcc/cp/contracts.h : /build_implicit_handler_member_check/
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-handler-member-* : *
