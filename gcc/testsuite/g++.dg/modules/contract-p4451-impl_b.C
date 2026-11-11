// Implementation unit of contract-p4451-impl_a.C: the interface's
// import of std precedes the assertion-control-specifier.
// { dg-additional-options "-fmodules -fcontracts-p4451" }

module p4451m;

static int g (int x) pre<terminating> (x > 0) { return x; }

int f (int x)
{
  contract_assert<terminating> (x > 0);
  return g (x);
}
