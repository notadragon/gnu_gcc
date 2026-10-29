# GCC-734: `-fsanitize=vptr` does not check the reference form of a `static_cast` downcast

**Status:** Fixed here
**Resolved by:** `0400-ubsan-vptr-reference-downcast`
**Component:** sanitizer
**Keywords (ours -- upstream sets its own):** `wrong-code` (missed check)
**Upstream Link:** [PR116102](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116102)
(NEW; not ours).
**Affects:** measured 2026-10-13 -- 13.4.0, 16.2.0 and the stock trunk
nightly `17.0.0 20261005` report the pointer form and not the reference
form; fixed on this branch.  Clang's `-fsanitize=vptr` reports both.

## Reproducer

[`gcc-734-vptr-reference-downcast.cpp`](gcc-734-vptr-reference-downcast.cpp),
at `-fsanitize=vptr`, run:

    gcc-734-vptr-reference-downcast.cpp:6:47: runtime error: downcast of address 0x... which does not point to an object of type 'T'

and nothing for line 7, the same downcast written `static_cast<T &> (r)`.
`static_cast<T &&>` is missed the same way.

## Cause

The reference path of `build_static_cast_1` (`gcc/cp/typeck.cc`) calls
`cp_ubsan_maybe_instrument_downcast` with the operand's class type `B` as
the source type, which that function rejects because it is not a pointer
or reference, so nothing is instrumented.  It also calls it before
converting the operand from `B *` to `D *`, where the check reads the
vptr as a `D`'s.

## Our Fix

The check is made after `build_base_path`, on the `D *`, with `B *` as the
source type, as the pointer form does.  If `build_base_path` tested for
null (`-fno-delete-null-pointer-checks`), only the adjusted pointer is
instrumented, so `-fsanitize=null` still finds the `COND_EXPR` it checks
for a null reference.  Under `-fcontracts-p3100` the report is routed to
the contract-violation handler like every vptr report
(`ub:expr.static.cast.base.class`).

Tests: `g++.dg/ubsan/vptr-reference-downcast.C` (lvalue, xvalue and const
references, a base at a nonzero offset, and casts to the object's own type
or a base of it, which do not report) and
`g++.dg/ubsan/p3100-vptr-downcast-ref-route-*.C`.

## Found by

The D4277R1 per-UB review (F15), which found the gap measuring the
reference-form celink; the upstream report predates it.
