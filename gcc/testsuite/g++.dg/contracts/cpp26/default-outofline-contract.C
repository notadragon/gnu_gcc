// An out-of-line explicitly defaulted function whose defaulting declaration
// repeats the first declaration's contracts: cp_finish_decl synthesizes it,
// so the repeated predicates must be parsed before then.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

struct S {
  int i = 0;
  S () = default;
  S &operator= (const S &) pre (i >= 0) post (r: &r == this);
  bool operator== (const S &) const pre (i >= 0);
};
S &S::operator= (const S &) pre (i >= 0) post (r: &r == this) = default;
bool S::operator== (const S &) const pre (i >= 0) = default;

int main ()
{
  S a, b;
  a.i = -1;
  b = a;			// b.i == 0: OK; now b.i == -1
  if (violations != 0)
    __builtin_abort ();
  a = b;			// a.i == -1
  if (violations != 1)
    __builtin_abort ();
  (void) (a == b);		// a.i == -1
  if (violations != 2)
    __builtin_abort ();
}
