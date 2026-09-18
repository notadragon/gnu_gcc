// gcc-33-class-result-binding-by-reference.cpp                      -*-C++-*-
//
// GCC-33: a postcondition whose result binding has CLASS type ICEs at codegen
// when the predicate passes it to a function taking a reference.
//
//   g++ -std=c++26 -fcontracts -c gcc-33-class-result-binding-by-reference.cpp
//
// Stock g++ trunk: internal compiler error during codegen.  -fsyntax-only is
// NOT enough to show it; the ICE is past the front end.
//
// PLAIN -fcontracts.

struct Vec { };

bool ok (const Vec &) { return true; }

// THE BUG.
Vec f () post (r : ok (r)) { return {}; }

// CONTROL: a result binding not passed anywhere is fine, so it is the
// reference binding and not the class-typed result that matters.
Vec g () post (r : true) { return {}; }

// CONTROL: a SCALAR result passed the same way is fine, so it is the class
// type and not the passing.
bool ok_int (const int &) { return true; }
int h () post (r : ok_int (r)) { return 0; }

// The reporter's original had TWO postconditions and concluded both were
// needed.  They are not -- the single one above is enough, which is worth
// saying in the report.
Vec two () post (r : true) post (r : ok (r)) { return {}; }

int
main ()
{
  f ();
  g ();
  h ();
  two ();
}
