/* PR c++/128021 -- -g changed the order in which the caller-side contract
   wrappers were emitted (and, optimizing, the order of their violation
   objects), so -fcompare-debug failed.  The wrappers were defined by
   walking a table hashed by the address of each callee's declaration, and
   -g allocates more trees, which moves those addresses.  The contracts
   tables hash a declaration by its uid (as for a precompiled header), so
   the walk no longer depends on where the trees are allocated.  Many
   callees, so that a regression cannot pass by chance.  */
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce -fcontracts-client-check=all -Og -g -fcompare-debug" }

int f0 (int a) pre (a > 0) { return a; }
int f1 (int a) pre (a > 1) { return a; }
int f2 (int a) pre (a > 2) { return a; }
int f3 (int a) pre (a > 3) { return a; }
int f4 (int a) pre (a > 4) { return a; }
int f5 (int a) pre (a > 5) { return a; }
int f6 (int a) pre (a > 6) { return a; }
int f7 (int a) pre (a > 7) { return a; }
int f8 (int a) pre (a > 8) { return a; }
int f9 (int a) pre (a > 9) { return a; }
int f10 (int a) pre (a > 10) { return a; }
int f11 (int a) pre (a > 11) { return a; }
int f12 (int a) pre (a > 12) { return a; }
int f13 (int a) pre (a > 13) { return a; }
int f14 (int a) pre (a > 14) { return a; }
int f15 (int a) pre (a > 15) { return a; }
int f16 (int a) pre (a > 16) { return a; }
int f17 (int a) pre (a > 17) { return a; }
int f18 (int a) pre (a > 18) { return a; }
int f19 (int a) pre (a > 19) { return a; }
int f20 (int a) pre (a > 20) { return a; }
int f21 (int a) pre (a > 21) { return a; }
int f22 (int a) pre (a > 22) { return a; }
int f23 (int a) pre (a > 23) { return a; }

int
main ()
{
  int r = 0;
  r += f0 (1);
  r += f1 (1);
  r += f2 (1);
  r += f3 (1);
  r += f4 (1);
  r += f5 (1);
  r += f6 (1);
  r += f7 (1);
  r += f8 (1);
  r += f9 (1);
  r += f10 (1);
  r += f11 (1);
  r += f12 (1);
  r += f13 (1);
  r += f14 (1);
  r += f15 (1);
  r += f16 (1);
  r += f17 (1);
  r += f18 (1);
  r += f19 (1);
  r += f20 (1);
  r += f21 (1);
  r += f22 (1);
  r += f23 (1);
  return r;
}
