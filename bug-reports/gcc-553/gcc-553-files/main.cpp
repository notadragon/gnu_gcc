// g++ -std=c++26 -fmodules -fcontracts -fcontract-evaluation-semantic=observe \
//     -c m.cpp
// g++ -std=c++26 -fmodules -fcontracts -fcontract-evaluation-semantic=observe \
//     main.cpp m.o && ./a.out
//
// Expected: a contract violation after each of "g1", "TS" and "nn".  GCC
// 16.2 and trunk report only the one for nn; the templates instantiated
// here have lost their contracts.
import m;
#include <cstdio>
int main ()
{
  std::puts ("g1"); g1 (-1);
  std::puts ("TS"); TS<int>{}.k (-1);
  std::puts ("nn"); nn (-1);
  std::puts ("end");
}
