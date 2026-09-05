// Watch test for an OPEN bug: forming the address of a subobject before its
// construction has begun is accepted in a constant expression.
//
// [class.cdtor]/1: "For an object with a non-trivial constructor, referring to
// any non-static member or base class of the object before the constructor
// begins execution results in undefined behavior."  So neither `f` below is a
// core constant expression and both static_asserts must be errors.
//
// Tracked as GCC-2 in this repository's bug-reports/.  This file exists so
// that the day GCC starts diagnosing it, the xfail turns into an XPASS and we
// are told -- rather than finding out by re-running a reproducer by hand.
//
// The MIRROR of this file is
// clang/test/Contracts/OpenBugs/address-before-ctor-vbase.cpp and
// .../address-before-ctor-member.cpp in the llvm_llvm-project fork.  The two
// shapes are split there because they do NOT have the same status on Clang:
// Clang diagnoses the virtual-base one and accepts the member one, so only
// the second is xfailed there.  Same test content, per-compiler expectation.
//
// NO CONTRACTS ARE INVOLVED; this is pure core-language constexpr, found only
// because contracts work led here.
//
// { dg-do compile { target c++26 } }

// ---- Virtual base.  Clang gets this one right; GCC does not. ------------
namespace vbase {
  struct W { int j; };
  struct X : virtual W { };
  // Declaration ORDER is load-bearing: `p` must come first, so that `x` is
  // genuinely unbuilt when `p` is initialised.  With `x` first the program is
  // perfectly legal and the test would pin nothing.
  struct Y {
    int *p;
    X x;
    constexpr Y () : p (&x.j) { }   // X -> W vbase conversion before x is built
  };
  constexpr int f () { Y y; return y.p != nullptr; }
  static_assert ((f (), true)); // { dg-error "non-constant condition|not a constant expression|before its lifetime" "GCC-2 vbase (open)" { xfail *-*-* } }
}

// ---- Direct member with a non-trivial constructor.  Both compilers accept
//      this one, so both mirrors are xfailed. --------------------------------
namespace direct_member {
  struct Z {
    int k;
    constexpr Z () : k (0) { }      // user-provided -> non-trivial
  };
  struct Y {
    int *p;
    Z z;
    constexpr Y () : p (&z.k) { }   // must be an error: z is not yet built
  };
  constexpr int f () { Y y; return y.p != nullptr; }
  static_assert ((f (), true)); // { dg-error "non-constant condition|not a constant expression|before its lifetime" "GCC-2 direct member (open)" { xfail *-*-* } }
}
