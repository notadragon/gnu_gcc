// P3097: Feature-test macro — bumps __cpp_contracts to 202606L.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097" }

#if __cpp_contracts < 202606L
#error "__cpp_contracts not bumped to 202606L with -fcontracts-p3097"
#endif
