// An [[assume]] whose operand cannot be evaluated without side effects must
// be declined, not rejected: [dcl.attr.assume] says the operand is not
// evaluated, so an implementation that cannot evaluate it cleanly simply
// does not get to assume anything.  g++ rejects the program instead.
//
// No headers, so this file is also its own preprocessed source.

constexpr bool modifying (int *p) { contract_assert ((++*p, true)); return true; }

constexpr int f ()
{
  int i = 0;
  [[assume (modifying (&i))]];   // operand is not evaluated
  return i;                      // therefore: 0
}

constexpr int v = f ();          // g++: error: contract condition is not constant
                                 // expected: v == 0

// The suppression itself is already correct -- only the reporting is wrong.
// Under a non-terminating semantic the same translation unit compiles and
// yields 0:
//
//   g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe
//     warning: contract condition is not constant
//     v == 0

int main () { return v; }
