// The evaluation semantic of a contract assertion can differ between
// translation units, also between the definitions of one inline function.
// Here the other unit enforces the assertion in f and its caller must not
// fold "f (y) > 0" to true from that: the linker keeps this unit's copy of
// f, which ignores the assertion and returns its argument.  If it kept the
// other copy, the violation handler runs and ends the program.
// { dg-lto-do run }
// { dg-lto-options { { -O2 -std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore -fno-ipa-sra } { -O2 -std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore -fno-ipa-sra -flto } } }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  std::exit (0);
}

[[gnu::noinline]] inline int
f (int x)
{
  contract_assert (x > 0);
  return x;
}

int (*volatile fp) (int) = f;

int use_e (int);

int
main ()
{
  if (use_e (-5) != 0)
    __builtin_abort ();
}
