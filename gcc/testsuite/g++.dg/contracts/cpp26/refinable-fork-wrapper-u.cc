// Extra translation unit of refinable-fork-wrapper.C: the definitions of g
// and h the callers here are compiled with, and the callers.  g checks
// f's postcondition caller-side (enforce), h's P3097 wrapper checks vf's
// postcondition (enforce).

int f (int x) post (r: r > 0);

struct B
{
  virtual int vf (int x) post (r: r > 0);
};

[[gnu::noinline]] inline int
g (int x)
{
  return f (x);	// caller-side enforce
}

[[gnu::noinline]] inline int
h (B *p, int x)
{
  return p->vf (x);
}

int use_g (int y) { return g (y) > 0; }
int use_h (B *p, int y) { return h (p, y) > 0; }
