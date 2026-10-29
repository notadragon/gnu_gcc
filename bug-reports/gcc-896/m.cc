// GCC-896, main.  g++ -std=c++26 -fcontracts -O2 -c m.cc
// g++ -std=c++26 -fcontracts m.o e.o i.o   (e.o first: the linker keeps E's f)
// ./a.out     expected "use_catch=-1"
// ./a.out x   expected "caught dtors=1"
#include <cstdio>
int use_catch (int); int use_dtor (int);
int dtors;
struct D { ~D (); };
D::~D () { ++dtors; }
int main (int argc, char **)
{
  if (argc == 1) { std::printf ("use_catch=%d\n", use_catch (-5)); return 0; }
  try { use_dtor (-5); } catch (int) { std::printf ("caught dtors=%d\n", dtors); }
}
