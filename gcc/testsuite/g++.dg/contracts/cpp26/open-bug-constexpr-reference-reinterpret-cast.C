// Watch test for an open bug (GCC-713, stock GCC too; not a contracts
// bug): a reinterpret_cast is not allowed in a core constant expression
// ([expr.const]), but its reference form is accepted, both as the
// initializer of a constexpr reference and inside a constexpr function.
// When it is fixed, the xfailed dg-error lines report an XPASS.
//
// Mirror: llvm_llvm-project
// clang/test/Contracts/constexpr-reference-reinterpret-cast.cpp, where Clang
// rejects both.
// { dg-do compile { target c++26 } }

float g = 1.0f;
constexpr int &r = reinterpret_cast<int &> (g);	// { dg-error "(reinterpret_cast|not a constant expression)" "GCC-713" { xfail *-*-* } }
constexpr bool
f ()
{
  float x = 1.0f;
  int &i = reinterpret_cast<int &> (x);
  (void) i;
  return true;
}
static_assert (f ());		// { dg-error "non-constant condition" "GCC-713" { xfail *-*-* } }

// Control, which passes: the pointer form is rejected.
constexpr bool
h ()
{
  float x = 1.0f;
  int *i = reinterpret_cast<int *> (&x);	// { dg-error "reinterpret_cast" }
  (void) i;
  return true;
}
static_assert (h ());		// { dg-error "(non-constant condition|called in a constant expression)" }
