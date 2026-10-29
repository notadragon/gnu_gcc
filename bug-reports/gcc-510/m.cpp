// A PCH's contract evaluation semantic silently overrides the TU's.
//   g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore -x c++-header h.h -o h.h.gch
//   g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -Winvalid-pch -include h.h m.cpp && ./a.out
//     -> "returned"   (the precondition is not checked)
//   rm h.h.gch; the same second command -> terminates (precondition checked)
#include <cstdio>
int main () { f (-1); std::puts ("returned"); }
