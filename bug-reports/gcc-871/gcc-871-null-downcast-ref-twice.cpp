#include <cstdio>
struct V { virtual void h () {} int v; };
struct X { virtual void x () {} long pad; };
struct D0 : V { int d; };              // V at offset 0
struct D2 : X, V { int d; };           // V at offset 16
__attribute__((noipa)) D0 &bind0 (V *p) { return static_cast<D0 &> (*p); }
__attribute__((noipa)) D2 &bind (V *p) { return static_cast<D2 &> (*p); }
int main ()
{
  std::setvbuf (stdout, nullptr, _IONBF, 0);
  V *volatile p = nullptr;
  std::printf ("bind\n");
  bind (p);
  std::printf ("bind0\n");
  bind0 (p);
  std::printf ("done\n");
}
