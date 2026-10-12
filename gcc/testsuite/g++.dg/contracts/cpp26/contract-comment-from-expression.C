// -fcontract-comment-from-expression forms the comment of a violation by
// printing the predicate rather than reading its source text, so the
// spelling written here (x>0) does not reach the comment.  The printed form
// is the processed predicate's (with its constification conversions), which
// this does not pin beyond its operator.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe -fcontract-comment-from-expression" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>
#include <cstring>

int violations;

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  if (std::strcmp (v.comment (), "x>0") == 0
      || std::strstr (v.comment (), " > 0") == nullptr)
    std::abort ();
  ++violations;
}

int f (int x) pre (x>0) { return x; }

int main ()
{
  int x = -1;
  f (x);
          contract_assert (x>0);
  if (violations != 2)
    std::abort ();
}
