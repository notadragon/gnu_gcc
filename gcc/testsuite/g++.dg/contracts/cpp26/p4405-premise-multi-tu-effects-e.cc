// Extra translation unit of p4405-premise-multi-tu-effects.C: f's shadow
// inlines h, whose precondition is ignored here, but f calls it out of line
// (the parameters provoke that, as in p4405-premise-shadow-only.C).

[[gnu::const]] int sinkc (int);
int big (int);
extern int g;

inline int h (int x) pre (x > 0)	// ignore
{
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sinkc (i * x);
  return s + sinkc (x) + sinkc (x + 1);
}

int f (int x, int y)
{
  contract_assert (x > 0 && x < 1000);	// assume
  if (x <= 0)
    {
      big (1); big (2); big (3); big (4); big (5); big (6);
      big (7); big (8); big (9); big (10); big (11); big (12);
      big (13); big (14); big (15);
    }
  g = 1;
  int r = h (y);
  int t = g;
  contract_assert (t == 1);	// diagnose: conditional, not assumed
  return r + (t == 1 ? 10 : 20);
}

