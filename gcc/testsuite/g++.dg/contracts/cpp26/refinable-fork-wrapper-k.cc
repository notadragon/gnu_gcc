// Extra translation unit of refinable-fork-wrapper.C, linked first: the
// definitions of f and B::vf, whose postconditions are ignored, and the
// definitions of g and h the linker keeps: g checks nothing caller-side,
// h's P3097 wrapper ignores vf's postcondition.

int f (int x) post (r: r > 0);

struct B
{
  virtual int vf (int x) post (r: r > 0);
};

int f (int x) { return x; }
int B::vf (int x) { return x; }

[[gnu::noinline]] inline int
g (int x)
{
  return f (x);
}

[[gnu::noinline]] inline int
h (B *p, int x)
{
  return p->vf (x);
}

int (*volatile keep_g) (int) = g;
int (*volatile keep_h) (B *, int) = h;
B b;
B *bp = &b;
