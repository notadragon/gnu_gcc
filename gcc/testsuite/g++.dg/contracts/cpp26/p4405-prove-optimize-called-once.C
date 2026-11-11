// D4405's proof optimization level (DECISIONS.md E162-E168; GCC-999,
// fixed): a proof at a level other than the build's inlines what the
// level's inliner would.  A -O0 build proves at level 2 through level
// copies of the callees the shadows reach, which nothing else calls; the
// inliner inlines a function into all its callers -- helper, a static
// function called once, too large for the small-function inliner; add1,
// under -fno-inline-small-functions -- after its first removal of
// unreachable functions, which keeps what only the shadows call until then
// (contract_shadow_boundary_hold).  So both assertions are proven at -O0
// and -O1 at level 2 as at -O2, and at level 1 in an -O2 build as in an
// -O1 build.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O0" }
// { dg-additional-options "-fcontract-evaluation-semantic=diagnose" }

int ext (int);

static int helper (int v)
{
  int s = v;
  for (int i = 0; i < 4; ++i)
    s += ext (i) * ext (s);
  s += ext (s) + ext (s + 1) + ext (s + 2) + ext (s + 3) + ext (s + 4);
  s += ext (s) + ext (s + 1) + ext (s + 2) + ext (s + 3) + ext (s + 4);
  s += ext (s) + ext (s + 1) + ext (s + 2) + ext (s + 3) + ext (s + 4);
  s += ext (s) + ext (s + 1) + ext (s + 2) + ext (s + 3) + ext (s + 4);
  return v < 0 ? 0 : v + (s & 1);
}

int q (int v)
{
  int c = helper (v);
  contract_assert (c != v - 1);	// { dg-bogus "cannot be proven" }
  return c;
}

int add1 (int v) { return v + 1; }

int r (int v)
{
  int c = add1 (v);
  contract_assert (c != v);	// { dg-bogus "cannot be proven" }
  return c;
}

// { dg-final { contracts-same-diagnostics "" "-O2" "-O1" "-O0 -fno-inline-small-functions" "-O1 -fno-inline-small-functions" "-O2 -fno-inline-small-functions" "-O3 -fcontracts-prove-optimize=2 -fno-inline-small-functions" "-O1 -fcontracts-prove-optimize=1" "-O2 -fcontracts-prove-optimize=1" } }
