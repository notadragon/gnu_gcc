---
id: 6300-p3100-check-pure-virtual
subject: 'c++: contracts: implicit pure-virtual-call check'
depends:
  - 6000-p3100-core
regenerates: []
fixes: []
---

## Rationale

The implicit contract assertion `ub:class.abstract.pure.virtual`: calling a
pure virtual function through the vtable, which [class.abstract] makes
undefined.

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
- gcc/cp/contracts.cc : @build_implicit_pure_virtual_terminus
- gcc/cp/contracts.h : @build_implicit_pure_virtual_terminus
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-pure-virtual-* : *
