// gcc-02-constexpr-vbase-before-ctor.cpp                            -*-C++-*-
//
// GCC-2: a derived-to-virtual-base conversion on an object whose
// construction has NOT BEGUN is accepted in a constant expression.
//
//   g++ -std=c++26 -fsyntax-only gcc-02-constexpr-vbase-before-ctor.cpp
//     -> accepted silently (no diagnostic, exit 0)
//
// It should be rejected, but NOT for the reason first recorded here.  The
// rule is [class.cdtor]/1: "For an object with a non-trivial constructor,
// referring to any non-static member or base class of the object before the
// constructor begins execution results in undefined behavior."  `X`'s default
// constructor is non-trivial (it has a virtual base), so forming `&x.j`
// before `x` is constructed is undefined and hence not a core constant
// expression.  There is no carve-out for virtual versus non-virtual bases,
// and none for forming an address versus reading.
//
// THE ORIGINAL RATIONALE WAS WRONG FOR GCC -- it said "computing the virtual
// base offset consults the most-derived object's layout, which is a use of
// the object".  That describes Clang's implementation, not GCC's.  Here the
// most-derived type is statically known, so GCC folds the conversion to a
// constant offset: the genericized tree is a plain COMPONENT_REF chain,
// `&((struct Y *) this)->x.D.2677.j`, with no vtable or VTT lookup at all.
// Nothing consults the object, so there is no dynamic-type operation on which
// a liveness check could hang.  Clang rejects this shape only because our
// CLANG-1 fix put a liveness check on its vbase-offset path.
//
// AND BOTH COMPILERS UNDER-DIAGNOSE THE GENERAL CASE (measured 2026-08-25):
// with the same "form the address before construction begins" shape, a
// NON-virtual base member and a plain direct member of a class with a
// non-trivial constructor are accepted by GCC *and* Clang alike.  Reading the
// member rather than taking its address is rejected by both.  So this file
// captures the one shape where the two disagree, not the boundary of the
// defect.  See `gcc-02-constexpr-vbase-before-ctor.md` in this directory for
// the table and for the scope decision owed before filing.
//
// NO CONTRACTS ARE INVOLVED.  This is a pure core-language constexpr
// evaluator issue.
//
// Clang rejects it:
//   error: static assertion expression is not an integral constant expression
//   note: dynamic_cast of object outside its lifetime is not allowed in a
//         constant expression
// (Clang only started rejecting it after llvm 8b33919f7aea, our own fix for
// the mirror-image Clang bug -- tracked as CLANG-1 (and, for the full-gap
// report, CLANG-9) in the `llvm_llvm-project` fork.  GCC and Clang were BOTH
// under-diagnosing here; only Clang has been fixed.)
//
// Not a trunk regression.  Older GCC (13.3) rejects this program, but for an
// unrelated reason -- it did not allow virtual base classes in a constexpr
// constructor at all ("'struct X' has virtual base classes").  The
// under-diagnosis became observable only once that restriction was lifted.

struct W { int j; };
struct X : virtual W { };

struct Y {
  int *p;
  X x;
  constexpr Y () : p (&x.j) { }   // X -> W vbase conversion before x is built
};

constexpr int f () { Y y; return y.p != nullptr; }

static_assert ((f (), true));     // GCC accepts; should be an error
