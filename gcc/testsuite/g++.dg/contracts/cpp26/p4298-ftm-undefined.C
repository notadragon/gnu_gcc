// E2: negative gate -- without -fcontracts-p4298 the nonthrowing-semantics
// feature-test macro is not defined (the umbrella -fcontracts-p3850 enables it,
// so use a bare -fcontracts here).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

#ifdef __cpp_contracts_nonthrowing_semantics
#error "__cpp_contracts_nonthrowing_semantics defined without -fcontracts-p4298"
#endif

int f (int x) pre (x > 0) { return x; }
int main () { return f (1); }
