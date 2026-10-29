---
id: 6420-p3100-handler-checks-clobber
subject: 'contracts: P3100: a check that may call the handler may clobber memory'
depends:
  - 6110-p3100-check-null-and-alignment
  - 6220-p3100-check-bounds
regenerates: []
fixes: []
---

## Rationale

The sanitizer checks are internal functions -- `IFN_UBSAN_NULL`,
`IFN_UBSAN_BOUNDS`, `IFN_UBSAN_PTR`, `IFN_UBSAN_VPTR`,
`IFN_UBSAN_OBJECT_SIZE` -- until `sanopt` expands them, and
`call_may_clobber_ref_p_1` treats them as pure for aliasing: a stock
sanitizer's report writes no memory the program cares about.  An implicit
contract assertion whose reaction calls the contract-violation handler, or
a sanitizer check routed to the handler, does: the handler is user code
and may change any object the program can reach.  Treated as pure, the
check lets the optimizers keep a value loaded before it, so at -O1 and
above the code after a check that reported and continued does not see what
the handler did -- a static only the handler changes keeps its old value.

So the alias oracle treats such a call like any other call when
`ubsan_ifn_may_call_handler_p` holds: an `IFN_UBSAN_NULL` whose null or
alignment reaction has a handler entry (the reactions that call the
handler), or a check of a sanitizer whose resolved `-fsanitize-semantic=`
is `noexcept_observe` or `noexcept_enforce`.  The decision is per call
for the implicit assertions, whose reactions are operands, so a check
that traps or does nothing keeps the old treatment, and stock sanitizer
code is unchanged.  The checks that call the handler entry directly
(division by zero, signed overflow) are ordinary calls and never had the
problem.  The address and thread checks are inserted by `pass_asan` and
`pass_tsan` after those optimizers have run, so a routed one that reports
and continues is outside this guarantee -- a documented limitation of
`-fsanitize-semantic=`; the `open-bug-routed-handler-effects-late.C` watch
tests pin it.

Across a call the same holds: such a check is a call of user code that has
no call graph edge until `sanopt` expands it, so a function whose only call
is a check (its callee deleted as const) was taken by `ipa-reference` to
read and write no static, and its caller reused a value loaded before the
call.  `ubsan_node_may_call_handler_p` says when a function may hold such
a check, from its references (the address of a handler entry point, an
`IFN_UBSAN_NULL` operand) and the routed sanitizers whose checks are
internal calls, since the body is not available at link time with -flto;
`ipa-reference` then takes the function to read and write every static, as
it does a call of an unknown function.

The routed checks that call a `__ubsan_handle_*` function directly
(shift, division, and every check the runtime reports through one) or are
`IFN_UBSAN_CHECK_ADD` / `_SUB` / `_MUL` (signed overflow) have the same
problem by other means: the built-ins are declared `leaf`, so a call is
taken to read and write none of the unit's statics, and the internal
functions are `ECF_CONST` until RTL expansion.  Routed, neither holds.
`flags_from_decl_or_type` drops `ECF_LEAF` from a built-in for which
`ubsan_builtin_may_call_handler_p` holds (its sanitizer routed with a
semantic that calls the handler), and `gimple_call_flags` drops
`ECF_CONST` and `ECF_LEAF` from a signed-overflow check for which
`ubsan_check_ifn_may_call_handler_p` holds, so the check gets virtual
operands, is not deleted when its value is unused, and clobbers memory;
`simplify_internal_call_using_ranges` unlinks those operands when it
replaces a check the ranges prove cannot overflow.
`ubsan_node_may_call_handler_p` counts the signed-overflow checks among
the internal calls.  Stock sanitizer code is unchanged: nothing differs
unless a check is routed.

## Compile gap

None: `UBSAN_NULL_ENTRY` / `UBSAN_NULL_ALIGN_ENTRY` and the declarations of
`ubsan_ifn_may_call_handler_p`, `ubsan_node_may_call_handler_p`,
`ubsan_builtin_may_call_handler_p` and `ubsan_check_ifn_may_call_handler_p`
are in `gcc/ubsan.h` (`6000-p3100-core`, which claims the whole file),
`resolved_sanitizer_semantic` and `explicit_sanitizer_semantic` in
`gcc/opts.h`, and the routed checks this names exist by
`6220-p3100-check-bounds`.

## Contents

- gcc/tree-ssa-alias.cc : *
- gcc/ubsan.cc : @ubsan_ifn_may_call_handler_p, @ubsan_routed_to_handler_p, @ubsan_node_may_call_handler_p, @ubsan_builtin_may_call_handler_p, @ubsan_check_ifn_may_call_handler_p, /^#include "opts\.h"$/
- gcc/calls.cc : *
- gcc/gimple.cc : *
- gcc/vr-values.cc : *
- gcc/ipa-reference.cc : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-handler-effects-inlined.C : *
- gcc/testsuite/g++.dg/ubsan/p3100-routed-handler-effects.C : *
- gcc/testsuite/g++.dg/asan/open-bug-routed-handler-effects-late.C : *
- gcc/testsuite/g++.dg/tsan/open-bug-routed-handler-effects-late.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-handler-effects-ipa.h : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-handler-effects-ipa.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-handler-effects-ipa-lto.C : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-handler-effects-ipa-O1.C : *
- gcc/testsuite/g++.dg/ubsan/p3100-routed-handler-effects-ipa.C : *
- gcc/testsuite/g++.dg/ubsan/p3100-routed-handler-effects-builtin.h : *
- gcc/testsuite/g++.dg/ubsan/p3100-routed-handler-effects-builtin.C : *
- gcc/testsuite/g++.dg/ubsan/p3100-routed-handler-effects-builtin-O1.C : *
- gcc/testsuite/g++.dg/ubsan/p3100-routed-handler-effects-builtin-O2.C : *
- gcc/testsuite/g++.dg/ubsan/p3100-routed-handler-effects-builtin-lto.C : *
- gcc/testsuite/gcc.dg/ubsan/p3100-routed-handler-effects-bitint.c : *
