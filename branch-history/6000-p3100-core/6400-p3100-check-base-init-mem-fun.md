---
id: 6400-p3100-check-base-init-mem-fun
subject: 'c++: contracts: P3100: implicit check of operations on *this before the bases are initialized'
depends:
  - 6390-p3100-check-handler-member
regenerates: []
fixes: []
---

## Rationale

The implicit contract assertion `ub:class.base.init.mem.fun`: calling a
member function of an object, or applying `typeid` or `dynamic_cast` to
it, before the mem-initializers for its base classes have completed or
after its bases have been destroyed, which [class.base.init] makes
undefined -- including in a precondition of its constructor and a
postcondition of its destructor.

The check is lexical and unconditional, for the direct forms: the
operation's object is `*this` written directly, and the operation is in a
base-class or delegating mem-initializer (`in_base_initializer` in a
constructor), or in a constructor precondition or destructor postcondition
(`contract_class_ptr`, which the parser sets for exactly those, or
`implicit_cdtor_contract_p`, which `tsubst_contract` sets for a template's).
`build_implicit_this_operation_check` prepends the violation, as
`6390-p3100-check-handler-member` does, at a member function call
(`build_over_call`, in that commit's `p3100_member_call_checks`: by name,
as an operator or as a conversion function), `typeid` of a polymorphic
`*this`
(`build_typeid`, whether or not the operand is evaluated: GCC knows the
dynamic type in a constructor) and `dynamic_cast` of a polymorphic `this`
or `*this` (`build_dynamic_cast`).

The leaves follow D4277R1: a virtual call, `typeid` and `dynamic_cast` are
`.polymorphic.virtual.call`, `.polymorphic.typeid` and
`.polymorphic.dynamic.cast`; any other member function call is
`.nonpolymorphic.mem.init` in a mem-initializer and
`.nonpolymorphic.contract` in a contract.  Constant evaluation is
unchanged.

## Compile gap

None: `implicit_this_object_p` and `build_implicit_unconditional_check`
are in `6390-p3100-check-handler-member`.

## Contents

- gcc/cp/call.cc : #7:1
- gcc/cp/rtti.cc : @build_typeid, @build_dynamic_cast, /#include "contracts\.h"/
- gcc/cp/pt.cc : /implicit_cdtor_contract_p/
- gcc/cp/contracts.cc : @implicit_cdtor_contract_p, @build_implicit_this_operation_check
- gcc/cp/contracts.h : /implicit_this_op/, /ITO_/, /implicit_cdtor_contract_p/, /build_implicit_this_operation_check/
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-member-operator-edges.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-base-init-mem-fun-* : *
