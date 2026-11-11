// A redeclaration whose postcondition differs from the first
// declaration's only in the presence of a result-name-introducer is
// ill-formed ([dcl.contract.func], the draft's own example).
//
//   g++ -std=c++26 -fcontracts -fsyntax-only redecl-result-name-presence.cpp
//
// Expected: an error on line 10.  GCC accepts it.
bool b1;
int g () post (r: b1);
int g () post (b1);             // error: mismatched result-name-introducer presence
