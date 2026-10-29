// The evaluation semantic of a contract assertion can differ between
// translation units, also between the definitions of one inline function.
// Here the other unit ignores the assertion in f, so its copy of f cannot
// throw, but its callers must not rely on that: the linker keeps this
// unit's copy, which enforces the assertion, and the violation handler
// throws.  The caller's catch must catch the exception and the caller's
// local must be destroyed.  Either copy may run; the result must match
// the one that did.
// { dg-lto-do run }
// { dg-lto-options { { -O2 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fno-ipa-sra } { -O2 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fno-ipa-sra -flto } } }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int handled;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++handled;
  throw 42;
}

[[gnu::noinline]] inline int
f (int x)
{
  contract_assert (x > 0);
  return x;
}

int (*volatile fp) (int) = f;

int use_catch (int);
int use_dtor (int);
extern int dtors;

int
main ()
{
  int r = use_catch (-5);
  if (handled ? r != -1 : r != -5)
    __builtin_abort ();
  int h = handled;
  try
    {
      r = use_dtor (-5);
      if (handled != h || r != -5)
	__builtin_abort ();
    }
  catch (int)
    {
      if (handled == h)
	__builtin_abort ();
    }
  if (dtors != 1)
    __builtin_abort ();
}
