// Extra translation unit of p4405-premise-multi-tu-effects.C: the same inline
// h, its precondition observed here, and this TU's copy emitted.

[[gnu::const]] int sinkc (int);

inline int h (int x) pre (x > 0)	// observe
{
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sinkc (i * x);
  return s + sinkc (x) + sinkc (x + 1);
}

// Emit this TU's copy of h.
int (*use_h) (int) = h;

