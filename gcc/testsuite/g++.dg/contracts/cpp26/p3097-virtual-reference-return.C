// GCC-678: the P3097 interface wrapper of a virtual function returning a
// reference returned the call to the final overrider as an expression of
// reference type, which finish_return_stmt does not expect: an ICE in
// convert_like_internal on a checking build (a release build accepted it).
// The wrapper must return the same object the final overrider does.
//
// Mirror: owed to Clang and EDG, recorded in their open-issues/README.md.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

struct B
{
  int m = 1;
  virtual int &get (int x) pre (x > 0) { return m; }
  virtual const int &cget (int x) const pre (x > 0) { return m; }
  virtual int &&rget (int x) pre (x > 0) { return static_cast<int &&> (m); }
  virtual ~B () {}
};

struct D : B
{
  int n = 2;
  int &get (int) override { return n; }
  const int &cget (int) const override { return n; }
  int &&rget (int) override { return static_cast<int &&> (n); }
};

int
main ()
{
  D d;
  B &b = d;
  if (&b.get (1) != &d.n || &b.cget (1) != &d.n || violations != 0)
    __builtin_abort ();
  b.get (0) = 5;
  if (d.n != 5 || violations != 1)
    __builtin_abort ();
  int &&r = b.rget (0);
  if (&r != &d.n || violations != 2)
    __builtin_abort ();
}
