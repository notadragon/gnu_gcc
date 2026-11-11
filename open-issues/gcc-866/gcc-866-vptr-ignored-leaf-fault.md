# GCC-866: with the null leaf ignored, GCC's vptr check is stock GCC's

**Kind:** divergence, accepted by the user 2026-10-17 (DECISIONS.md E119 (a))
**Status:** Accepted
**Affects:** measured 2026-10-17 on the install of the M88-GCC tree,
-O0 and -O2, `-fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe`
with `ub:expr.unary.dereference.nullptr:ignore`
**Tests:** `g++.dg/ubsan/p3100-null-vptr-offset-zero-ignore.C` pins it
**Clang:** `Runnable/p3100-null-vptr-offset-zero.cpp`, NONE run

## Symptom

With the null leaf ignored, the vptr check of a member call through a null
pointer to a polymorphic class -- `mp->W2::w2 ()`, `mp->nw ()`,
`((*mp).*p) ()`, `hp->m.nv ()`, `(mp->*p) ()` -- reads the vtable pointer
through the null pointer, as stock GCC's `-fsanitize=vptr` alone does.
Clang's vptr check skips a null pointer (stock), and with CLANG-797's fix
also for an object at offset zero.

## Cause

GCC-856's guard (`p3100_vptr_guard_pointer`, DECISIONS.md E116 (a)) skips
a null object pointer only where a leaf of
`ub:expr.unary.dereference.nullptr` applies: the null dereference is that
check's to report.  Otherwise the vptr check is GCC's stock one, which
tests no null pointer.

## Decision

E119 (a): with the null check ignored, GCC's vptr check stays its stock
one; the divergence from Clang is recorded, not owed work.
