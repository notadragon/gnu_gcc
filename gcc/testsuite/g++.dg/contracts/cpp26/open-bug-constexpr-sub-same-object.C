// Watch test for an open bug (GCC-740, stock GCC too, PR126060; not a
// contracts bug): subtracting pointers to two members of one object is
// undefined ([expr.add]/5.3: they are not elements of one array), but
// constant evaluation accepts it.  When it is fixed, the xfailed dg-error
// line reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-sub-same-object.cpp, where Clang rejects it.
// { dg-do compile { target c++26 } }

struct M { int a, b; };
constexpr long f () { M m{}; return &m.b - &m.a; }
static_assert (f () == 1);	// { dg-error "non-constant condition" "GCC-740" { xfail *-*-* } }
