// P4451 [basic.contract.control]: a declaration of
// std::contracts::labels::terminating shall precede each
// assertion-control-specifier.  <cassert> does not declare it; <contracts>
// does, from where it is included on.  Under -fcontracts-p4451 alone.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p4451" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <cassert>

void f (int x) pre<terminating> (x > 0);	// { dg-error "no declaration of .std::contracts::labels::terminating. precedes" }
struct S { void m (int x) pre<terminating> (x > 0); };	// { dg-error "no declaration of .std::contracts::labels::terminating. precedes" }
void g (int x)
{
  contract_assert<terminating> (x > 0);	// { dg-error "no declaration of .std::contracts::labels::terminating. precedes" }
}

#include <contracts>

void h (int x) pre<terminating> (x > 0);
struct T { void m (int x) pre<terminating> (x > 0); };
void k (int x)
{
  contract_assert<terminating> (x > 0);
}
