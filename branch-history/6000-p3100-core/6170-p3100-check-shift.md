---
id: 6170-p3100-check-shift
subject: 'c++: contracts: implicit shift-out-of-range check (RUC_SHIFT_BASE, RUC_SHIFT_EXPONENT)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check ids 8 and 9 -- `-fsanitize=shift-base` and
`-fsanitize=shift-exponent` -- and the implicit contract assertion
`ub:expr.shift.neg.and.width` that covers both.

The two wire ids are one commit because the language-level check is one
thing.  [expr.shift] makes a shift undefined when the right operand is
negative or not less than the width of the promoted left operand; the front
end resolves one group id for that, `build_implicit_shift_check` builds one
guard for it, and the tests configure one semantic for it.  UBSan splits the
condition into two `-fsanitize=` bits because it wants to name the operand
that was at fault in its report, but nothing on the contracts side can
select one and not the other.  Splitting them would produce two commits that
each hold one `.def` row and neither of which holds the check.

`build_implicit_shift_check` wraps the shift in a guard that yields a
defined value (the raw left operand) when the shift amount is out of range,
so `ignore` changes codegen rather than merely suppressing a report --
exactly as it must, since the point of the semantic is that the operation
becomes defined.

## Compile gap

Its caller is a forward reference in the other direction: the site that
calls `build_implicit_shift_check` is in `cp_build_binary_op`, and that
function's hunks resolve the divide, divide-overflow and shift semantics
together in one block and apply all three guards in one block.  The net diff
emits those as three hunks that name all three checks, so they cannot be
split, and they travel with `6180-p3100-check-integer-divide` -- the later
of the two in wire order, so that the caller never precedes the callee.

The consequence is precise and worth stating: after this commit
`build_implicit_shift_check` is defined, declared in `cp/contracts.h`, and
called by nobody.  It builds; a shift is simply never guarded, and
`-fcontract-implicit-semantic=ub:expr.shift.neg.and.width=observe` is
accepted and silently does nothing.  The routing half (the two `.def` rows)
IS live from this commit, so `p3100-shift-exponent-route-observe.C` passes
here while the `p3100-shift-oob-*` tests need `6180`.

Everything else it needs -- `resolve_implicit_contract_semantic`,
`build_implicit_op_guard`, the group registry -- is in `6000-p3100-core`.

## Contents

- gcc/contracts-routed-checks.def : /RUC_SHIFT_BASE/, /RUC_SHIFT_EXPONENT/
- gcc/cp/contracts.cc : @build_implicit_shift_check
- gcc/cp/contracts.h : @build_implicit_shift_check
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-shift-oob* : *
- gcc/testsuite/g++.dg/ubsan/p3100-shift-exponent-* : *
