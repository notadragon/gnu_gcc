// Watch test for an open bug (GCC-749, stock GCC too, PR115592 (CWG2823),
// PR126360, PR113047; not a contracts bug): indirection through a null
// pointer is undefined ([expr.unary.op]/1, CWG2823) whether or not the
// result is read, but constant evaluation accepts it when nothing is read:
// a discarded `*p`, its address `&*p`, and a static member function called
// or a static data member named through a null pointer (the object
// expression is evaluated); for a null pointer to function too, `&*fp` and
// a discarded `*fp;` (stock GCC 13.4..trunk).  Each is folded or dropped at
// parse time.  A reference bound to `*p` or `*fp` and a call through a null
// pointer to function are rejected (the controls).  When it is fixed, the
// xfailed dg-error lines report an XPASS.
//
// The member forms are also CLANG-737 in the llvm_llvm-project fork (Clang
// accepts `sp->sf ()` and `sp->k` too, and rejects `*p;` and `&*p`); the
// function forms are CLANG-756 (Clang accepts the reference binding too).
// Mirror: llvm_llvm-project
// clang/test/Contracts/OpenBugs/constexpr-null-static-member-call.cpp,
// clang/test/Contracts/constexpr-null-indirection-no-read.cpp and
// clang/test/Contracts/OpenBugs/constexpr-null-function-indirection.cpp.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-Wno-unused-value" }

struct S { static constexpr int sf () { return 1; } static constexpr int k = 1; };
constexpr bool
f1 ()
{
  int *p = nullptr;
  *p;
  return true;
}
static_assert (f1 ());	// { dg-error "non-constant condition" "GCC-749" { xfail *-*-* } }
constexpr bool
f2 ()
{
  S *sp = nullptr;
  sp->sf ();
  return true;
}
static_assert (f2 ());	// { dg-error "non-constant condition" "GCC-749" { xfail *-*-* } }
constexpr bool
f3 ()
{
  S *sp = nullptr;
  return sp->k == 1;
}
static_assert (f3 ());	// { dg-error "non-constant condition" "GCC-749" { xfail *-*-* } }

// The address of `*p`, and the forms for a null pointer to function.
using F = int ();
constexpr bool oaddr (S *p) { return &*p == nullptr; }
static_assert (oaddr (nullptr));	// { dg-error "non-constant condition" "GCC-749" { xfail *-*-* } }

constexpr bool addr (F *fp) { return &*fp == nullptr; }
static_assert (addr (nullptr));	// { dg-error "non-constant condition" "GCC-749" { xfail *-*-* } }

constexpr int disc (F *fp) { *fp; return 1; }
static_assert (disc (nullptr) == 1);	// { dg-error "non-constant condition" "GCC-749" { xfail *-*-* } }

// Controls, which pass.
constexpr int bind (F *fp) { F &r = *fp; (void) r; return 1; }	// { dg-error "dereferencing a null pointer" }
static_assert (bind (nullptr) == 1);	// { dg-error "non-constant condition" }

constexpr int call (F *fp) { return (*fp) (); }	// { dg-error "does not designate" }
static_assert (call (nullptr) == 1);	// { dg-error "non-constant condition" }

constexpr int obind (S *p) { S &r = *p; (void) r; return 1; }	// { dg-error "dereferencing a null pointer" }
static_assert (obind (nullptr) == 1);	// { dg-error "non-constant condition" }
