# GCC-711: ICE in `cxx_eval_constant_expression`, constexpr.cc:10816, diagnosing a `static_cast` from `void*` whose operand is `a + N`

**Status:** Open here; open upstream
**Component:** c++
**Upstream Link:** [PR125199](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125199)
**Affects:** stock trunk `17.0.0 20261005` (the CE nightly at `8802cd9fdbe`, a checking build), this branch's release install of 2026-10-12
**Bugzilla Summary:** (existing PR125199)

## Reproducer

[`gcc-711-ice-void-cast-pointer-plus.cpp`](gcc-711-ice-void-cast-pointer-plus.cpp), at `-std=c++26 -fsyntax-only`:

    constexpr bool f() {
      int a[2]{};
      void* v = a + 1;
      unsigned* p = static_cast<unsigned*>(v);
      return p;
    }
    static_assert(f());

Stock trunk: `internal compiler error: in cxx_eval_constant_expression, at
cp/constexpr.cc:10816` -- `gcc_assert (TREE_CODE (sop) == ADDR_EXPR)` in the
P2738 `void*`-cast diagnostic, reached with a `POINTER_PLUS_EXPR` operand.
Release builds (16.2.0, 15.3.0, this branch) print "confused by earlier
errors, bailing out".  `&a[1]` gets the correct "cast from 'void*' is not
allowed" error.  Also reached by placement `new (s + 1) int()` on a `char`
array in constant evaluation.  PR125199's own reduced case hits the same line.

Found by the D4277R1 per-UB review (2026-10-12).
