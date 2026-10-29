# GCC-712: ICE in `synthesized_method_walk`, method.cc:3031, explaining why an implicit destructor with a virtual base is not constexpr

**Status:** Open here; open upstream
**Component:** c++
**Upstream Link:** [PR123681](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=123681)
**Affects:** stock trunk `17.0.0 20261005` (the CE nightly at `8802cd9fdbe`, a checking build), this branch's release install of 2026-10-12
**Bugzilla Summary:** (existing PR123681)

## Reproducer

[`gcc-712-ice-synthesized-method-walk.cpp`](gcc-712-ice-synthesized-method-walk.cpp), at `-std=c++26 -fsyntax-only`:

    struct B { virtual ~B(); };
    struct D : virtual B {};
    constexpr int t() { D* d = new D(); delete d; return 0; }
    static_assert(t() == 0);

Stock trunk: after the (correct) error, `internal compiler error: in
synthesized_method_walk, at cp/method.cc:3031`.  Release builds (16.2.0, this
branch) print "confused by earlier errors, bailing out".  PR123681's testcase
takes the same path.

Found by the D4277R1 per-UB review (2026-10-12).
