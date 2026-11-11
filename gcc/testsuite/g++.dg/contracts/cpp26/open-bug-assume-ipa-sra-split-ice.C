// Watch test for an open bug (GCC-952, stock GCC too; not a contracts bug):
// a precondition evaluated with the assume semantic is outlined like an
// [[assume]], and when it reads a data member of a member function local
// enough for ipa-sra to split `this', modify_call asserts on the .ASSUME
// call's argument map.  The program is valid and should compile.  When it
// is fixed, dg-ice reports an XPASS; then drop -fno-ipa-sra from
// p3100-semantic-assume-run.C.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-allow-assume -fcontract-evaluation-semantic=assume -O2" }
// { dg-ice "GCC-952" }
// { dg-prune-output "during IPA pass" }

[[gnu::noinline]] bool check (int i) { return i > 0; }

struct S
{
  int v = 1;
  [[gnu::noinline]] int m (int i) pre (check (i + v)) { return i + v; }
};

int main ()
{
  S s;
  return s.m (2) != 3;
}
