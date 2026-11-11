# GCC-713: A reference `reinterpret_cast` is accepted in a constant expression

**Status:** Open here; open upstream
**Component:** c++
**Upstream Link:** None found (searched 2026-10-12) -- filing owed; related PR92411 (pointer, identity case)
**Affects:** stock trunk `17.0.0 20261005` (the CE nightly at `8802cd9fdbe`, a checking build), this branch's release install of 2026-10-12
**Bugzilla Summary:** reinterpret_cast to a reference type accepted in a constant expression

## Reproducer

[`gcc-713-constexpr-reference-reinterpret-cast.cpp`](gcc-713-constexpr-reference-reinterpret-cast.cpp), at `-std=c++26 -fsyntax-only`:

    float g = 1.0f;
    constexpr int& r = reinterpret_cast<int&>(g);   // not a core constant expression
    constexpr bool f() { float x = 1.0f; int& i = reinterpret_cast<int&>(x); (void)i; return true; }
    static_assert(f());

Accepted by stock trunk, 16.2.0 through 13.4.0, and this branch; Clang
rejects ("reinterpret_cast is not allowed in a constant expression").
[expr.const] excludes every `reinterpret_cast`.  The pointer form is rejected.
Because of this, GCC's constant evaluator reaches -- and evaluates through --
mistyped references (`ub:dcl.ref.incompatible.type`,
`ub:dcl.ref.incompatible.function`, `ub:expr.ref.member.not.similar`): a call
through `int(&)(int)` bound to `int f(float)` even evaluates.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

File with the **Bugzilla Summary** above as the summary and the
Reproducer section (code, flags and observed behavior) as the description.

Found by the D4277R1 per-UB review (2026-10-12).
