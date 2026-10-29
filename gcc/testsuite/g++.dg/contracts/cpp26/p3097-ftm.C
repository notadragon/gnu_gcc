// P3097: Feature-test macro -- bumps __cpp_contracts past P2900's 202502L.
// The value is the date of the implemented revision, which changes with
// every revision, so only check that it is later than P2900's.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097" }

#if __cpp_contracts <= 202502L
#error "__cpp_contracts not bumped past 202502L with -fcontracts-p3097"
#endif
