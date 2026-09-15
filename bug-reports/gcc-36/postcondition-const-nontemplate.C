// A non-template function reports the same [dcl.contract.func]
// const-parameter diagnostic twice -- the identical message, at the
// identical location, with the identical note.
//
//   g++ -std=c++26 -fsyntax-only postcondition-const-nontemplate.C

int f(int v) post(r : v == r) { return v; }
