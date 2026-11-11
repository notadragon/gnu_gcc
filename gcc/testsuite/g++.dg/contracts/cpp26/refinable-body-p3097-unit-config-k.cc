// Extra translation unit of refinable-body-p3097-unit-config.C, linked
// first: the definitions of g and ng the linker keeps, whose P3097
// wrappers check B::f's and B::nf's preconditions with enforce.

struct B
{
  virtual int f (int x) pre (x > 0);
  virtual int nf (int x) noexcept pre (x > 0);
  virtual ~B () = default;
};

[[gnu::noinline]] inline int g (B &b, int x) { return b.f (x); }
[[gnu::noinline]] inline int ng (B &b, int x) { return b.nf (x); }

int (*volatile keep_g) (B &, int) = g;
int (*volatile keep_ng) (B &, int) = ng;
