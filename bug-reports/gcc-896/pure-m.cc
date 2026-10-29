// GCC-896 (memory form), main.  g++ -std=c++26 -fcontracts -O2 -c pure-m.cc
// g++ -std=c++26 -fcontracts pure-m.o pure-e.o pure-i.o
// expected "r=0 calls=1"
#include <cstdio>
int use_g (int); extern int calls;
int main () { int r = use_g (-5); std::printf ("r=%d calls=%d\n", r, calls); }
