# GCC-715: rejects-valid: a pointer to member converted to a base of the containing class is not a constant expression when applied

**Status:** Open here; open upstream
**Component:** c++
**Upstream Link:** None found (searched 2026-10-12) -- filing owed; PR104678 and PR114031 are different shapes
**Affects:** stock trunk `17.0.0 20261005` (the CE nightly at `8802cd9fdbe`, a checking build), this branch's release install of 2026-10-12
**Bugzilla Summary:** rejects-valid: pointer to member static_cast to a base of the containing class not usable in a constant expression

## Reproducer

[`gcc-715-rejects-valid-ptrmem-cast-to-base.cpp`](gcc-715-rejects-valid-ptrmem-cast-to-base.cpp), at `-std=c++26 -fsyntax-only`:

    struct Bb { int i; };
    struct C : Bb { int m; };
    constexpr int f() {
      int Bb::*p = static_cast<int Bb::*>(&C::m);  // OK [expr.static.cast]
      C c{};
      c.m = 5;
      return c.*p;                                  // OK: c contains C::m
    }
    static_assert(f() == 5);

Stock trunk, 14.4.0 onwards, and this branch reject ("non-constant condition
for static assertion"; note: `'*(((int*)(& c.C::Bb)) + ((sizetype)p))' is not
a constant expression`).  Clang accepts.  The member pointer is tracked as a
bare offset from the `Bb` subobject, so applying it leaves that subobject.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

File with the **Bugzilla Summary** above as the summary and the
Reproducer section (code, flags and observed behavior) as the description.

Found by the D4277R1 per-UB review (2026-10-12).
