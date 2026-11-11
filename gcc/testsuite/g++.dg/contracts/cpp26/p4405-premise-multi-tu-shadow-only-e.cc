// Extra translation unit of p4405-premise-multi-tu-shadow-only-1.C and -2.C:
// g's shadow inlines h (the parameters provoke it, as in
// p4405-premise-shadow-only.C) but g calls it out of line, and h's precondition
// is enforced here.

int sink (int);
int big (int);

inline int h (int x) pre (x > 0)	// enforce
{
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}

int g (int x, int y)
{
  contract_assert (x > 0 && x < 1000);	// assume
  if (x <= 0)
    {
      big (1); big (2); big (3); big (4); big (5); big (6);
      big (7); big (8); big (9); big (10); big (11); big (12);
      big (13); big (14); big (15);
    }
  int r = h (y);
  contract_assert (y > 0);	// diagnose: conditional, not assumed
  return r + (y > 0 ? 1 : 0);
}

