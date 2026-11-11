---
id: 6300-p3100-check-pure-virtual
subject: 'c++: contracts: P3100: implicit pure-virtual-call check'
depends:
  - 6000-p3100-core
regenerates: []
fixes: []
---

## Rationale

The implicit contract assertion `ub:class.abstract.pure.virtual`: calling a
pure virtual function through the vtable, which [class.abstract] makes
undefined.  A call that reaches the vtable slot is the group's `.indirect`
leaf; a direct call in the class's own constructor or destructor is resolved
statically and never reaches it (`.direct`).

This check has no routed-check id and no `-fsanitize=` bit -- there is no
UBSan check for it, so there is nothing to route and this commit depends on
no sanitizer.  It is nonetheless a distinct UB check with its own group id,
its own semantics and its own tests, which is why it is its own commit.

The implementation is unlike every other check on the branch because it is
not instrumentation at a site: `build_vtbl_initializer` points the vtable
slot at a contract-aware terminus instead of `__cxa_pure_virtual` when the
class's contract configuration selects a checking semantic, and
`build_implicit_pure_virtual_terminus` builds that function.  The check
therefore lives in the class's vtable, and the semantic is resolved once
per class rather than once per call.

That is a real consequence and not an implementation detail: the
configuration that applies is the one in force where the vtable is emitted,
not the one at the call, and the diagnostic reports the location of the
class, not of the caller.

The `.direct` leaf is checked at the call instead.  When the front end
resolves an unqualified virtual call statically to a pure virtual function
-- `*this` in a constructor or destructor of a class with no final
overrider for it -- running the call would run the pure virtual's
definition, or fail to link without one.
`build_implicit_pure_virtual_direct_check`, from `build_over_call` where it
resolves the call statically (`p3100_devirtualized`), prepends the violation
to such a call -- a member function call, an operator or a conversion
function -- resolved at the call site.  It
reacts as the terminus does: "assume" and "ignore" keep the call,
"observe" aborts after the handler returns, and a noexcept pure virtual
promotes a throwing semantic to its noexcept form.  The violation is a
guard on a constant true marked as not a constant expression, so constant
evaluation is unchanged.

## Compile gap

None: `build_implicit_op_guard`, `resolve_implicit_contract_semantic` and
the group registry are in `6000-p3100-core`, and the terminus's declaration
travels here with its definition.

The trap is the one-vtable-one-semantic property above.  Two translation
units that configure this check differently and both emit the class's vtable
produce two different terminus functions with the same mangled vtable
symbol; the linker keeps one, and which one it keeps decides the semantic
for the whole program, with no diagnostic.  A reviewer should read
`build_vtbl_initializer` with that in mind -- it is the reason the terminus
is selected from the class's configuration rather than the current
function's.

## Contents

- gcc/cp/class.cc : @build_vtbl_initializer
- gcc/cp/contracts.cc : @build_implicit_pure_virtual_terminus, @build_implicit_pure_virtual_direct_check
- gcc/cp/contracts.h : @build_implicit_pure_virtual_terminus, /build_implicit_pure_virtual_direct_check/
- gcc/cp/call.cc : /build_implicit_pure_virtual_direct_check/, /p3100_devirtualized/
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-pure-virtual-* : *
