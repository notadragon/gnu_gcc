// A refinable COMDAT definition preempted by a definition outside the LTO
// IR keeps its body for inlining, like an available one.
// { dg-lto-do run }
// { dg-lto-options { { -O2 -fno-early-inlining -flto --param=ipa-refinable-inline=1 } } }

int v = 1;

inline int f () { return v; }

int
main ()
{
  // F is inlined at link time from this unit's body; the definition the
  // symbol binds to returns 2.
  return f () == 1 ? 0 : 1;
}
