// F30: in constant evaluation, a non-terminating (observe) contract violation
// must not mask a later terminating (enforce) violation.  Whether an ill-formed
// program is rejected must not depend on left-to-right evaluation order.
// Namespace A resolves to observe (config file); everything else (B) is enforce.
// Evaluating A::obs(-1) (which fails, observe) first must NOT swallow the
// subsequent B::enf(-1) enforce failure -- the initializer must still be
// ill-formed.  Before the fix the enforce was short-circuited and `bad` was
// accepted with only a warning.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3850 -fcontract-evaluation-semantic=enforce -fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/contract-constexpr-observe-masks-enforce.json" }
#include <contracts>

namespace A { constexpr int obs (int x) { contract_assert (x > 0);   return x; } }
namespace B
{
  constexpr int
  enf (int x)
  {
    contract_assert (x > 100); // { dg-error "contract predicate is false in constant expression" }
    return x;
  }
}

constexpr int bad = A::obs (-1) + B::enf (-1);
