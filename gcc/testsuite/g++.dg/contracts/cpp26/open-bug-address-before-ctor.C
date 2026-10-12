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
// The mirror of this file is
// clang/test/Contracts/OpenBugs/address-before-ctor-member.cpp in the
// llvm_llvm-project fork, for the member shape only: Clang diagnoses the
// virtual-base shape, which clang/test/SemaCXX/constexpr-vbase-lifetime.cpp
// covers, and accepts the member one, so only the member shape is xfailed
// there.  Same test content, per-compiler expectation.
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

// ---- Through a pointer to the unbuilt member (the EDG fork's EDG-45
//      shapes).  Both compilers accept it, so both mirrors are xfailed. -----
namespace through_pointer {
  struct Z {
    int k;
    constexpr Z () : k (0) { }
  };
  struct Y {
    Z *p;
    int *r;
    Z z;
    constexpr Y () : p (&z), r (&p->k) { }  // &z is fine; &p->k is not
  };
  static_assert ((Y (), true)); // { dg-error "non-constant condition|not a constant expression|before its lifetime" "GCC-2 through a pointer (open)" { xfail *-*-* } }
}

// ---- Controls, which pass: reading the unbuilt member is diagnosed; the
//      object's own address, a member with a trivial constructor, an
//      already-built member and the constructor body are fine. ------------
namespace controls {
  struct Z {
    int k;
    constexpr Z () : k (0) { }
  };
  struct Y2 { int q; Z z; constexpr Y2 () : q (z.k) { } };  // { dg-error "accessing uninitialized member" }
  constexpr int f2 () { Y2 y; return y.q; }
  static_assert ((f2 (), true)); // { dg-error "non-constant condition" }
  struct Y4 { Z *p; Z z; constexpr Y4 () : p (&z) { } };
  static_assert ((Y4 (), true));
  struct T { int k; };
  struct Y5 { int *p; T t; constexpr Y5 () : p (&t.k), t{1} { } };
  static_assert ((Y5 (), true));
  struct Y6 { Z z; int *p; constexpr Y6 () : p (&z.k) { } };
  static_assert ((Y6 (), true));
  struct Y7 { Z z; int k; constexpr Y7 () : k (0) { z.k = 1; } };
  static_assert ((Y7 (), true));
}
