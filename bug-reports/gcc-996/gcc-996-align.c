/* GCC-996: compile with -O1 -S.  g, compiled at -O1, gets the function and
   loop alignment of -O2 (.p2align 4 before g, and before its loop) that f's
   optimize attribute set, because nothing resets it for g.  Without the
   attribute on f, g has no .p2align at all.  */
int sink (int);
__attribute__ ((optimize ("O2"))) int f (int n) { int s = 0; for (int i = 0; i < n; ++i) s += sink (i); return s; }
int g (int n) { int s = 0; for (int i = 0; i < n; ++i) s += sink (i); return s; }
