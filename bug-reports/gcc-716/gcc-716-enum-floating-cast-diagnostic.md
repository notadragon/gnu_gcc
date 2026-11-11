# GCC-716: `static_cast` of an out-of-range floating value to an unfixed enumeration is rejected with an unhelpful diagnostic

**Status:** Open here; open upstream
**Component:** c++
**Upstream Link:** None found (searched 2026-10-12) -- filing owed
**Affects:** stock trunk `17.0.0 20261005` (the CE nightly at `8802cd9fdbe`, a checking build), this branch's release install of 2026-10-12
**Bugzilla Summary:** poor diagnostic for out-of-range floating-point static_cast to enumeration in a constant expression

## Reproducer

[`gcc-716-enum-floating-cast-diagnostic.cpp`](gcc-716-enum-floating-cast-diagnostic.cpp), at `-std=c++26 -fsyntax-only`:

    enum A { a0, a1, a2, a3 };                 // values [0, 3]
    constexpr A x = static_cast<A>(4.0);       // 4.0 -> int 4 -> A: undefined

Stock trunk and this branch: "'(unsigned int)4.0e+0' is not a constant
expression".  The rejection is right ([expr.static.cast] converts the
floating value to the underlying type and then to the enumeration, and 4 is
outside [0, 3]) but the message names neither the enumeration nor the range.
16.2.0 accepts.  The integral form `static_cast<A>(4)` is accepted (PR95701).
Clang accepts the floating form (a separate Clang constant-evaluation miss).

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

File with the **Bugzilla Summary** above as the summary and the
Reproducer section (code, flags and observed behavior) as the description.

Found by the D4277R1 per-UB review (2026-10-12).
