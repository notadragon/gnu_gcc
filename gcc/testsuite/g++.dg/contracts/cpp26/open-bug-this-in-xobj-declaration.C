// Watch test for an OPEN bug: `this` is accepted within the declaration of an
// explicit object member function.
//
// [expr.prim.this]/3, as amended by P0847R7: `this` "shall not appear within
// the declaration of either a static member function or an explicit object
// member function of the current class".  One sentence, two kinds of
// function; the static half IS diagnosed, which is what shows only half of it
// is being applied.
//
// Tracked as GCC-17 in this repository's bug-reports/, and as CLANG-8 in the
// llvm_llvm-project fork.  This file exists so that the day the remaining row
// starts being diagnosed, its xfail becomes an XPASS and we are told.
//
// Measured 2026-09-05, and the two compilers do NOT agree, which is why the
// mirrors carry different expectations row by row:
//
//   row                          GCC                Clang
//   xobj trailing return type    accepts (bug)      accepts (bug)
//   xobj noexcept-specifier      REJECTS (correct)  accepts (bug)
//   static, either               rejects (correct)  rejects (correct)
//   implicit object, either      accepts (correct)  accepts (correct)
//
// So only ONE row is xfailed here, while the Clang mirror xfails two -- split
// across two files there, because lit's XFAIL is per file where DejaGnu's is
// per line.  See clang/test/Contracts/OpenBugs/this-in-xobj-*.cpp.
//
// NO CONTRACTS ARE INVOLVED; plain C++23, found by a deducing-this sweep.
//
// { dg-do compile { target c++23 } }

struct S
{
  int x;
  static int s;

  // ---- explicit object member function ---------------------------------
  // Trailing return type: THE OPEN BUG on both compilers.
  auto f1 (this S &self) -> decltype (this->x); // { dg-error "invalid use of 'this'" "GCC-17 trailing return type (open)" { xfail *-*-* } }

  // noexcept-specifier: GCC gets this one right.  Kept because the Clang
  // mirror xfails it, and because a fix for the row above must not regress
  // this one.
  void f2 (this S &self) noexcept (noexcept (this->x)); // { dg-error "invalid use of 'this' at top level" }

  // ---- static member function: the control, correctly rejected today ---
  static auto g1 () -> decltype (this->x); // { dg-error "invalid use of 'this' at top level" }
  static void g2 () noexcept (noexcept (this->x)); // { dg-error "invalid use of 'this' at top level" }

  // ---- implicit object member function: must stay ACCEPTED -------------
  auto h1 () -> decltype (this->x);
  void h2 () noexcept (noexcept (this->x));
};
