# GCC-717: UBSan `vptr` reports a defined `static_cast` downcast performed during construction of the derived object

**Status:** Open here; open upstream
**Component:** c++
**Upstream Link:** None found (searched 2026-10-12) -- filing owed; Clang has the same false positive: llvm#59060
**Affects:** stock trunk `17.0.0 20261005` (the CE nightly at `8802cd9fdbe`, a checking build), this branch's release install of 2026-10-12
**Bugzilla Summary:** -fsanitize=vptr false positive for static_cast downcast during construction

## Reproducer

[`gcc-717-ubsan-vptr-downcast-during-construction.cpp`](gcc-717-ubsan-vptr-downcast-during-construction.cpp), at `-std=c++26 -O0 -fsanitize=vptr`:

    struct B { virtual ~B() {} };
    struct E : B { E(); };
    struct F : E { int x = 0; };
    E::E() { (void)static_cast<F*>(static_cast<B*>(this)); }  // defined while an F is being built
    int main() { F f; }

Stock trunk and this branch report "downcast of address ... which does not
point to an object of type 'F' ... object is of type 'E'".  The `B` is a base
subobject of the `F` under construction, so [expr.static.cast] gives the
enclosing `F`; both constant evaluators accept the constexpr form.  Under an
enforcing semantic routed through the integrated `vptr` check
(`ub:expr.static.cast.downcast.wrong.derived.type`,
`ub:expr.static.cast.base.class`) this terminates a valid program.

## Draft upstream report (GCC Bugzilla; filing owed -- the user files)

File with the **Bugzilla Summary** above as the summary and the
Reproducer section (code, flags and observed behavior) as the description.

Found by the D4277R1 per-UB review (2026-10-12).
