// gcc-12-result-binding-denotes-two-objects.cpp                      -*-C++-*-
//
// PR112794, sharpened.  A postcondition's result binding denotes DIFFERENT
// OBJECTS within a SINGLE evaluation of the predicate, depending on whether a
// reference to it is bound through a function parameter.
//
//   g++ -std=c++26 -fcontracts gcc-12-....cpp && ./a.out
//   -> "same object? NO"   (the two addresses differ, here by 4 bytes)
//
//   clang++ -std=c++26 -fcontracts gcc-12-....cpp && ./a.out
//   -> "same object? YES"
//
// WHY THIS FRAMING MATTERS.  The original PR112794 reproducer observes the
// mutation from the caller, which invites the counter-argument that
// [dcl.contract.res] Example 2 permits the implementation to introduce a
// temporary for a register-returned result -- so the caller not seeing the
// write would be unspecified rather than wrong.
//
// That defence does not reach this program.  Whichever object `r' is bound to
// -- the returned object or a temporary -- [dcl.contract.res]/1 binds it to
// ONE object, and it must stay that object for the whole predicate.  Here a
// single evaluation of a single predicate yields two addresses for `r'.  No
// latitude in [class.temporary] licenses that; it says which object may be
// used, not that there may be two.
//
// The mechanism, from the observable behaviour: binding a reference parameter
// to `r' makes the gimplifier spill the result to a fresh temporary, and the
// callee-side write lands there.  Taking `&r' directly does not spill.  So
// the identity of `r' varies with the syntax of the sub-expression that names
// it.
//
// The caller-visible symptom of the same defect, for reference -- three
// spellings of the same mutation of the same object in one program:
//
//   post (r : (const_cast<int&>(r) = 42, true))   -> caller sees 42
//   post (r : mutate (r))  [mutate(const int&)]   -> caller sees 0   <-- lost
//   post (r : (const_cast<int&>(r)++, true))      -> caller sees 1
//
// If a temporary had been introduced for the return value, that would be a
// property of the function and all three would agree.  They do not.
//
// PROVENANCE: UPSTREAM'S, measured 2026-09-02.  Reproduces on stock g++
// 16.2.0 and g++-trunk (17.0.0 20260901) as well as on our branch.  Clang
// gives one object in every spelling.
//
// *** STRONGER REPRODUCER, added 2026-09-02 -- LEAD WITH THIS ONE. ***
//
// The address-comparison above is recorded through side effects, and
// [basic.contract.eval] says it is "unspecified whether the predicate is
// evaluated" and that a side-effect-free alternative evaluation may be
// substituted -- so a defender could argue the recorded addresses prove
// nothing.  Put the comparison INSIDE the predicate and that defence is gone,
// because the disagreement becomes the predicate's VALUE:
//
//   bool same (const int &a, const int *b) { return &a == b; }
//   int f () post (r : same (r, &r)) { return 7; }
//
//   g++   : CONTRACT VIOLATION -- the predicate evaluates to false, and the
//           program aborts under the enforce semantic
//   clang : no violation; the program runs to completion
//
// A faithful evaluation of `same (r, &r)' is plainly true.  [basic.contract.eval]
// defines B as "the value that would result from evaluating the predicate" and
// says a contract violation occurs when "B is false".  B is true here, so no
// violation may occur.  The latitude in that paragraph is for "an alternative
// evaluation that produces THE SAME VALUE"; an evaluation producing false is
// not one.  So this is non-conforming with no wording question attached.
//
// NOTE the const_cast is well-formed and defined: the returned object is not
// a const object, so writing through a const_cast of a const lvalue that
// designates it has defined behaviour.  "A result binding is a const lvalue"
// ([dcl.contract.res]/1) makes `r = 42' ill-formed; it does not make this
// program ill-formed or undefined, and it does not settle PR112794.

#include <cstdio>

const int *a_direct = nullptr;
const int *a_viaref = nullptr;

const int *addr_via_ref (const int &i) { return &i; }

bool rec_direct (const int *p) { a_direct = p; return true; }
bool rec_viaref (const int *p) { a_viaref = p; return true; }

// Both operands name the SAME result binding, in ONE predicate evaluation.
// A contract predicate cannot assign to a variable it names, so the addresses
// are recorded through called functions.
int
f () post (r : rec_direct (&r) && rec_viaref (addr_via_ref (r)))
{
  return 7;
}

// The value-based form: the disagreement becomes the predicate's VALUE, so
// neither the Example 2 temporary argument nor the side-effect latitude of
// [basic.contract.eval] applies.  A faithful evaluation is plainly true.
bool
same (const int &a, const int *b)
{
  return &a == b;
}

int
value_form () post (r : same (r, &r))
{
  return 7;
}

int
main ()
{
  /* g++ aborts here with a contract violation; clang runs on.  */
  value_form ();

  int v = f ();

  std::printf ("value              = %d\n", v);
  std::printf ("&r taken directly  = %p\n", (const void *) a_direct);
  std::printf ("&r via const int&  = %p\n", (const void *) a_viaref);
  std::printf ("same object?         %s\n",
	       a_direct == a_viaref ? "YES" : "NO  <-- r denotes two objects");

  return a_direct == a_viaref ? 0 : 1;
}
