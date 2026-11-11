// Extra translation unit of p4405-premise-multi-tu-shadow-only-1.C and -2.C:
// the same inline h, its precondition ignored here.

[[gnu::noinline]] int sink (int v) { return v; }
[[gnu::noinline]] int big (int v) { return v; }

inline int h (int x) pre (x > 0)	// ignore
{
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}

// Emit this TU's copy of h.
int (*use_h) (int) = h;

