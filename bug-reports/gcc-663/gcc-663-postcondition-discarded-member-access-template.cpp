// GCC-663: a function template's parameter named only as the object of a
// discarded member access in a postcondition is required to be const on
// the definition.  Expected: clean.  -std=c++26 -fcontracts
struct S { int m; };

template <class T> void t (const T s) post ((s.m, true));
template <class T> void t (T s) {}

void u () { t (S{1}); }
