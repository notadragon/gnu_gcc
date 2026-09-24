// P3100: a null-dereference contract and an alignment contract configured at the
// SAME access must each fire with their own semantic -- enabling one must not
// disable the other.  Here null=noexcept_observe, align=noexcept_enforce; the
// access is non-null but misaligned, so the alignment check fires (sem=7) and
// terminates -- it is NOT suppressed by the coexisting null-deref contract.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-align-with-null.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// { dg-shouldfail "alignment enforce fires despite a coexisting null-deref contract" }

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation (const cs::contract_violation& v)
{
  std::printf ("VIOL sem=%d comment=[%s]\n",
	       (int) v.semantic (), v.comment ());
  std::fflush (stdout);
}
int __attribute__((noinline)) load (int* p) { return *p; }
int main ()
{
  alignas (int) char buf[8] = {1,2,3,4,5,6,7,8};
  int* p = reinterpret_cast<int*> (buf + 1);   // non-null but misaligned
  std::printf ("RESULT=%d\n", load (p));
  return 0;
}
// { dg-output "VIOL sem=7 comment=\\\[misaligned pointer access\\\]" }
