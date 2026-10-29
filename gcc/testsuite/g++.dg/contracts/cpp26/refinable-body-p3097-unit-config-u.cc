// Extra translation unit of refinable-body-p3097-unit-config.C: B::f and
// B::nf, the definitions of g and ng the callers here are compiled with,
// and the callers.  The preconditions are ignored here, so g and ng make
// no evaluation.  ng's call of a noexcept function cannot throw, so use_ng
// may drop its handler; g's may, so use_g keeps its.

struct B
{
  virtual int f (int x) pre (x > 0);
  virtual int nf (int x) noexcept pre (x > 0);
  virtual ~B () = default;
};

int B::f (int x) { return x; }
int B::nf (int x) noexcept { return x; }

[[gnu::noinline]] inline int g (B &b, int x) { return b.f (x); }
[[gnu::noinline]] inline int ng (B &b, int x) { return b.nf (x); }

int
use_g (int y)
{
  B b;
  try
    {
      return g (b, y);
    }
  catch (...)
    {
      return -1;
    }
}

int
use_ng (int y)
{
  B b;
  try
    {
      return ng (b, y);
    }
  catch (...)
    {
      return -1;
    }
}
