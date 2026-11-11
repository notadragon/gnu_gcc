// GCC-877: -fsanitize=alignment does not check the pointer of a placement
// new that stores nothing.  g++ -fsanitize=alignment, run.
#include <cstdio>
#include <new>
struct alignas (4) S {};
struct T { int a, b; };
__attribute__((noipa)) void empty (char *b) { std::fputs ("empty\n", stderr); new (b) S (); }
__attribute__((noipa)) void dflt (char *b) { std::fputs ("dflt\n", stderr); new (b) T; }
__attribute__((noipa)) void store (char *b) { std::fputs ("store\n", stderr); new (b) int (1); }
alignas (8) char buf[16];
int main () { empty (buf + 1); dflt (buf + 1); store (buf + 1); std::fputs ("done\n", stderr); }
