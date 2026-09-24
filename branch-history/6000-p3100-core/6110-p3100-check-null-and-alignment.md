---
id: 6110-p3100-check-null-and-alignment
subject: 'c++: contracts: P3100: implicit null-dereference and misaligned-access checks'
depends:
  - 6000-p3100-core
  - 6030-p3100-ubsan-runtime
regenerates: []
fixes: []
---

## Rationale

Routed-check ids 2 and 7 -- `-fsanitize=alignment` and `-fsanitize=null` --
together with the two implicit contract assertions that reuse their
machinery: `ub:expr.unary.dereference.nullptr` and
`ub:basic.align.object.alignment`.

These two checks are one commit because they are one indivisible change.
Both are carried by the same internal function, `IFN_UBSAN_NULL`, whose
operand list this branch widens from three operands to nine -- a null
reaction plus handler entry and data block, and the same three again for
alignment.  `instrument_mem_ref` resolves both reactions at `pass_ubsan`
(pre-inline, where `cfun->decl` is still the function whose namespace the
contract configuration matches) and writes both onto the one call;
`ubsan_expand_null_ifn` reads both back and, when either applies, splits the
alignment condition's true edge into its own then-block so a null contract
and an alignment contract at the same access each fire with their own
semantic.  A single `build_reaction` lambda serves both edges.  Nearly every
hunk in `ubsan_expand_null_ifn` and `instrument_mem_ref` names both
reactions; there is no cut that puts null in one commit and alignment in
another.

`c-ubsan.cc`'s `ubsan_maybe_instrument_reference_or_call` is the front-end
entry to the same IFN, for binding a reference and for the implied `this`
dereference of a member call, and carries the same two reactions.
`cp_genericize_r` opens the walk that reaches it when `-fcontracts-p3100` is
on but no sanitizer is.  `lto-streamer-in.cc` is the reason the reactions
are operands rather than a re-resolution: a `.UBSAN_NULL` carrying a
non-`IMPLICIT_UB_NONE` reaction must survive stream-in even with the
sanitizers off, and `-fcontracts-p3100` is a front-end flag that is not in
the LTO options section, so the flag is re-armed for the surviving call.

The `.def` row for `RUC_ALIGNMENT` is *not* here: it is one net-diff hunk
with the rows for object-size, nonnull-attribute and returns-nonnull-
attribute, so it stays in `6000-p3100-core` with the rest of the table's
indivisible text.  `RUC_NULL` is its own row and does travel here.

## Compile gap

Forward references that would need stubbing if this commit were built alone:
none.  The `UBSAN_NULL_*` operand enumeration and `enum
implicit_ub_reaction` are in `gcc/ubsan.h`, the `IFN_UBSAN_NULL` arity in
`gcc/internal-fn.def`, the `resolve_implicit_ub_semantic` and
`build_implicit_ub_handler` langhooks and their C++ implementations, and the
`pass_ubsan` gate that runs the pass under `-fcontracts-p3100`, are all in
`6000-p3100-core` -- as is `sanopt.cc`, which must already know that two
`IFN_UBSAN_NULL` calls may only be merged when their reactions match.

The semantic trap is the pre-inline resolution.  The reaction is resolved
once, in `pass_ubsan`, and carried on the IFN.  `ubsan_expand_null_ifn` runs
at `sanopt`, post-inline and -- under LTO -- at LTRANS where no C++ langhook
exists; re-resolving there would match the *caller's* namespace against the
contract configuration and silently drop or change the check.  Nothing
diagnoses that; the only symptom is a check that does not fire, which is why
`p3100-nullderef-two-ns.C` and `p3100-nullderef-lto.C` exist.

## Contents

- gcc/c-family/c-ubsan.cc : /#include "langhooks\.h"/, @ubsan_maybe_instrument_reference_or_call
- gcc/contracts-routed-checks.def : /RUC_NULL/
- gcc/cp/cp-gimplify.cc : /SANITIZE_ALIGNMENT/
- gcc/lto-streamer-in.cc : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-align-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-alignment-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-null-* : *
- gcc/testsuite/g++.dg/contracts/cpp26/p3100-nullderef-* : *
- gcc/testsuite/g++.dg/ubsan/p3100-alignment-* : *
- gcc/testsuite/g++.dg/ubsan/p3100-null-* : *
- gcc/ubsan.cc : @ubsan_expand_bounds_ifn, @implicit_null_deref_reaction, @implicit_align_reaction, @ubsan_expand_null_ifn, @instrument_mem_ref
