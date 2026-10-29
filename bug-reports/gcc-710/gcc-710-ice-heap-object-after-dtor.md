# GCC-710: ICE (checking) in `cxx_eval_constant_expression`, constexpr.cc:11356, on a constexpr use of a heap object after its destructor ran

**Status:** Fixed here; open upstream
**Component:** c++
**Upstream Link:** None found as an ICE (searched 2026-10-12) -- filing owed; PR107529's own testcase triggers it on a checking build; related PR126425
**Affects:** stock trunk `17.0.0 20261005` (the CE nightly at `8802cd9fdbe`, a checking build); not this branch (release and checking builds, 2026-10-13)
**Bugzilla Summary:** [17 Regression?] ICE (checking) in cxx_eval_constant_expression, at cp/constexpr.cc:11356, reading a heap object after its destructor

## Reproducer

[`gcc-710-ice-heap-object-after-dtor.cpp`](gcc-710-ice-heap-object-after-dtor.cpp), at `-std=c++26 -fsyntax-only`:

    struct T { int x = 0; };
    constexpr int f() { T* p = new T; p->~T(); int r = p->x; delete p; return r; }
    static_assert(f() == 0);

Stock trunk (checking): `internal compiler error: in
cxx_eval_constant_expression, at cp/constexpr.cc:11356` -- the
`gcc_checking_assert (false)` in the `r == void_node` fallback.  Release
16.2.0 rejects through the same fallback ("'heap [0]' accesses an object
outside its lifetime"); 15.3.0 and 14.4.0 accept.  The same ICE occurs for a
virtual call, a `dynamic_cast`, a virtual-base conversion, or `delete` through
a virtual destructor after an explicit destructor call on a heap object.

This branch rejects cleanly ("accessing an allocated object outside its
lifetime") because of the GCC-69/70 constexpr heap-lifetime fix, so the
fallback is not reached.  Confirmed on this branch's *checking* build
(`--enable-checking` dev tree at `ba0aa88608c`'s compiler sources,
2026-10-13): the reproducer, and the virtual-call, virtual-base-conversion
and delete-through-virtual-destructor variants, are rejected with no ICE
(`g++.dg/contracts/cpp26/constexpr-heap-after-dtor.C`, which passes on the
release and checking builds alike).  The `dynamic_cast` variant is not
rejected here either: it evaluates to a null pointer (GCC-752).

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

File with the **Bugzilla Summary** above as the summary and the
Reproducer section (code, flags and observed behavior) as the description.

Found by the D4277R1 per-UB review (2026-10-12).
