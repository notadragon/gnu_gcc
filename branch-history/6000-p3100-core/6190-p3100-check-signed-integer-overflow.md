---
id: 6190-p3100-check-signed-integer-overflow
subject: 'c++: contracts: P3100: implicit signed-integer-overflow check (RUC_SIGNED_INTEGER_OVERFLOW)'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check id 11, `-fsanitize=signed-integer-overflow`, and the implicit
contract assertion `ub:expr.expr.eval.signed.integer`.

The implementation is a new middle-end lowering,
`instrument_si_overflow_contract`, sitting beside the sanitizer's own.  It
rewrites a signed `+`, `-`, `*` or unary `-` into the matching
`.ADD_OVERFLOW` / `.SUB_OVERFLOW` / `.MUL_OVERFLOW` internal call, uses the
real part -- the defined two's-complement wrapped result --
unconditionally, and adds a very-unlikely branch on the overflow flag to the
configured reaction.  `ignore` is the same rewrite with no branch.

That is the whole point of the check and the reason it is not a report-only
routing: using an internal function for the operation also stops the
optimizer assuming the operation cannot overflow, so a loop built on it is
no longer treated as provably finite.  `ignore` and `observe` therefore
change generated code, which is the behaviour the paper asks for and which a
pure sanitizer route cannot give.

`instrument_si_overflow` is rewritten to take its iterator by pointer,
because the contract lowering can replace the statement and split the block
under it; the stock sanitizer path is unchanged apart from that.  The RTL
path in `ubsan_build_overflow_builtin` gains only a comment recording that
contract assertions never reach it.

## Compile gap

None: `implicit_overflow_reaction`, which resolves the site, is here; the
`resolve_implicit_ub_semantic` and `build_implicit_ub_handler` langhooks it
calls, `enum implicit_ub_reaction`, and the `pass_ubsan` driver that runs
the pass under `-fcontracts-p3100` and dispatches to
`instrument_si_overflow` are all in `6000-p3100-core`.

Two traps.  The resolution happens in `pass_ubsan`, which runs before
inlining, and the reaction is used immediately rather than carried on an
IFN as the null check carries it; that is safe only because the lowering is
completed in the same pass.  And the semantic is resolved against the
enclosing function's namespace, so a configuration keyed on a namespace
resolves differently before and after inlining -- doing this work anywhere
later would silently change which sites are checked.

## Contents

- gcc/contracts-routed-checks.def : /RUC_SIGNED_INTEGER_OVERFLOW/
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-overflow* : *
- gcc/testsuite/g++.dg/ubsan/p3100-signed-integer-overflow-* : *
- gcc/ubsan.cc : @implicit_overflow_reaction, @ubsan_build_overflow_builtin, @static, /tree_code code = gimple_assign_rhs_code \(stmt\);/, /^instrument_si_overflow \(gimple_stmt_iterator \*gsi\)$/, /When the sanitizer is off, only instrument for a P3100 implicit/, /gsi_replace \(&gsi, g, true\);/, /gsi_insert_before \(&gsi, g, GSI_SAME_STMT\);/
