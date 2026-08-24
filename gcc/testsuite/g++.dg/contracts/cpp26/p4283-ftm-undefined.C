// E3: negative gate -- without -fcontracts-p4283 the requires-on-contracts
// feature-test macro is not defined (the umbrella -fcontracts-p3850 enables it,
// so use a bare -fcontracts here).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

#ifdef __cpp_contracts_requires
#error "__cpp_contracts_requires defined without -fcontracts-p4283"
#endif

int f (int x) pre (x > 0) { return x; }
int main () { return f (1); }
