// Extra translation unit of p4405-premise-multi-tu-inlined.C: the same inline
// h, its precondition ignored, and this TU's copy emitted.

[[gnu::always_inline]] inline int h (int x) pre (x > 0)	// ignore
{
  return x;
}

int (*use_h) (int) = h;

