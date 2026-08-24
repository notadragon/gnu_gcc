// A4 / F30 (part 2): observe violations reported during constant evaluation are
// capped (constexpr_contract_violation_limit == 8); any beyond the cap are
// summarised as a count rather than reported individually.  Nine const-but-false
// observe violations in one evaluation -> the first eight warn, the ninth is
// counted ("and 1 more contract violation not shown").
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3850 -fcontract-evaluation-semantic=observe" }
#include <contracts>

constexpr int
f (int x)
{
  contract_assert (x > 0); // { dg-warning "contract predicate is false in constant expression" }
  // The summary note is attached to the first recorded violation's location.
  // { dg-message "and 1 more contract violation not shown" "cap summary" { target *-*-* } .-2 }
  contract_assert (x > 1); // { dg-warning "contract predicate is false in constant expression" }
  contract_assert (x > 2); // { dg-warning "contract predicate is false in constant expression" }
  contract_assert (x > 3); // { dg-warning "contract predicate is false in constant expression" }
  contract_assert (x > 4); // { dg-warning "contract predicate is false in constant expression" }
  contract_assert (x > 5); // { dg-warning "contract predicate is false in constant expression" }
  contract_assert (x > 6); // { dg-warning "contract predicate is false in constant expression" }
  contract_assert (x > 7); // { dg-warning "contract predicate is false in constant expression" }
  contract_assert (x > 8); // ninth violation: over the cap, counted not shown
  return x;
}

constexpr int bad = f (-1);
