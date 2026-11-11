// -fcontract-violation-information=full, the default: the data block of an
// implicit contract assertion whose check the middle end inserts (the null
// dereference check) names the file and line and has a comment.
// p3100-violation-information-none.C is the same under =none.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100 -fcontracts-p4298 -fcontract-evaluation-semantic=observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe" }
// { dg-additional-options "-fcontract-violation-information=full" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdlib>
#include <cstring>

using namespace std::contracts;

void handle_contract_violation (const contract_violation &v)
{
  if (std::strcmp (v.location ().file_name (), __FILE__) != 0
      || !std::strstr (v.location ().function_name (), "main")
      || v.location ().line () != 34
      || std::strcmp (v.comment (), "null pointer dereference") != 0
      || v.message () != nullptr
      || v.kind () != assertion_kind::implicit
      || v.semantic () != evaluation_semantic::noexcept_observe)
    __builtin_abort ();
  // The load would fault after an observed violation.
  std::_Exit (0);
}

int main ()
{
  int *volatile p = nullptr;
  volatile int r = *p;	// line 34
  (void) r;
  __builtin_abort ();
}
