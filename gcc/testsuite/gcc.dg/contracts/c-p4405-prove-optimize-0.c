/* D4405's proof optimization level in C (DECISIONS.md E162-E168, E205):
   at -fcontracts-prove-optimize=0 folding is the only strategy, so the
   postcondition c-p4405-shadow-post.c proves in its proof shadow is
   diagnosed, while one that folds is proven.  Pinned to -O2 (the torture
   options are skipped).  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -O2 -fcontracts-prove-optimize=0" } */
/* { dg-skip-if "pinned to -O2" { *-*-* } { "-O0" "-O1" "-O3" "-Os" "-Og" "-flto" } { "" } } */

int
clamped (int v)
  _Post (r: r >= 0 && r <= 9)	/* { dg-error "contract assertion cannot be proven" } */
{
  if (v < 0)
    return 0;
  if (v > 9)
    return 9;
  return v;
}

int
folded (int v)
  _Post (1)			/* { dg-bogus "(cannot be proven|always false)" } */
{
  return v;
}
