// Watch test for an open bug (GCC-712, stock GCC too, PR123681; not a
// contracts bug): explaining why the implicit destructor of a class with a
// virtual base is not constexpr ICEs -- in synthesized_method_walk on a
// checking build, in type_has_trivial_fn on a release one.  It is reached
// here through -Winvalid-constexpr, a warning, so no error precedes the ICE
// and a release compiler does not mask it as "confused by earlier errors"
// (the bug-reports/ reproducer, a static_assert, only shows the ICE on a
// checking build).  When it is fixed, dg-ice reports an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-vbase-dtor-non-constexpr.cpp.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-Winvalid-constexpr" }
// { dg-ice "GCC-712" }

struct B { virtual ~B (); };
struct D : virtual B {};
constexpr int t (D *d) { d->D::~D (); return 0; }	// { dg-warning "call to non-.constexpr. function" }
