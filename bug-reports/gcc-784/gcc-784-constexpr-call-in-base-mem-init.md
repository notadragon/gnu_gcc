# GCC-784: Member function call in a base mem-initializer not rejected in constant evaluation

**Status:** Open here; open upstream
**Component:** c++ (constexpr)
**Upstream Link:** [PR126357](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=126357) (the call; the `dynamic_cast` form is not shown there)
**Affects:** stock trunk `17.0.0 20261005` (`8802cd9fdbe`), 16.2.0, 15.3.0, 14.4.0; this branch's install of 2026-10-13
**D4277R1 category:** `ub:class.base.init.mem.fun`

## Reproducer

[`gcc-784-constexpr-call-in-base-mem-init.cpp`](gcc-784-constexpr-call-in-base-mem-init.cpp), at `-std=c++26 -fsyntax-only`:

    struct A { int a; constexpr A(int x) : a(x) {} };
    struct B : A {
      constexpr int g() const { return 1; }
      constexpr B() : A(g()) {}     // UB [class.base.init]/16
    };
    static_assert((B{}, true));

Accepted (the core undefined behavior is not diagnosed, so the expression
is wrongly treated as a constant expression).  `dynamic_cast<B *> (this)`
in the same mem-initializer (with a polymorphic `A`) is accepted too.  The
virtual call is GCC-758.  Clang accepts both as well (llvm_llvm-project
CLANG-713, llvm#211286).

## Upstream search (2026-10-13)

Bugzilla REST, control validated (the summary phrase "Calling member
function before base subobject" returned PR126357).  PR126357 is this bug's
call form exactly (UNCONFIRMED); nothing found for the `dynamic_cast` form
(`s "dynamic_cast base initializer"`, `s "dynamic_cast mem-initializer"`,
`s "dynamic_cast constructor constexpr"`).
A comment adding the `dynamic_cast` form there is the user's call.

Found by the CLANG-713 mirror (M52-GCC, 2026-10-13).
