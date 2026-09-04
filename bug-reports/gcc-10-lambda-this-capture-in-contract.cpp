// GCC-10 -- a contract predicate on a lambda that captures `this' reads the
// closure object as if it were the enclosing class.
//
//   g++ -std=c++26 -fcontracts gcc-10-lambda-this-capture-in-contract.cpp
//
// Expected: "predicate saw m = 2", exit 0.
// GCC:      some unrelated value (the low half of the captured __this
//           pointer), exit 1.
// Clang:    correct.
//
// Wrong code, not an ICE, and the symptom is nondeterministic if you only
// watch whether the check fires: the predicate compares against a stack
// address, so whether `x > m' happens to hold varies between builds.  This
// reproducer reports the value the predicate actually saw instead, which
// fails deterministically.
//
// The generated operator() reads
//
//   _1 = MEM[(struct S *)__closure].m;      // the predicate -- WRONG
//   _2 = __closure->__this; _3 = _2->m;     // the body      -- right
//
// so the predicate reinterprets the closure object as an S and reads
// whatever sits at offset 0.
//
// Root cause: remap_dummy_this_1 (gcc/cp/contracts.cc) rewrites every tree
// for which is_this_parameter is true to DECL_ARGUMENTS of the function
// being emitted into.  In a lambda's operator() that first argument is
// __closure, not an S*.  is_this_parameter (gcc/cp/semantics.cc:14768) is
// deliberately true for BOTH the real `this' PARM_DECL and a lambda's
// captured-`this' proxy -- a VAR_DECL named `this' whose DECL_VALUE_EXPR is
// already `__closure->__this'.  The proxy needs no remapping at all; the
// remap exists for the dummy `this' of a contract parsed on a declaration.
//
// PROVENANCE: upstream's.  Stock g++ 16.2.0 emits the identical
// `MEM[(struct S *)__closure].m' in the predicate, remap_dummy_this_1 is
// byte-identical in upstream/master, and the reproducer needs no extension
// of ours -- plain -fcontracts.

#include <cstdio>

static int seen = -1;

static bool
probe (int observed)
{
  seen = observed;
  return true;
}

struct S {
  int m;

  int through_this ()
  {
    // The predicate reads m through the captured `this'.
    auto l = [this] () pre (probe (m)) { return m; };
    return l ();
  }
};

int main ()
{
  S s { 2 };
  int body = s.through_this ();
  std::printf ("body saw m = %d, predicate saw m = %d\n", body, seen);
  if (seen != 2)
    {
      std::printf ("FAIL: predicate read the closure, not the object\n");
      return 1;
    }
  return 0;
}
