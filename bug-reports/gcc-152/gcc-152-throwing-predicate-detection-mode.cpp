// GCC-152 / PR125163: a contract predicate that exits via an exception is
// reported to the contract-violation handler with detection_mode
// predicate_false (1) instead of evaluation_exception (2).
//
// g++ -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe
//
// Expected output: "mode=2".  Stock GCC prints "mode=1".
//
// Stock <contracts> also spells the accessor mode() where
// [support.contract.violation] declares detection_mode(); the requires
// expression below lets the one reproducer compile against either.
#include <contracts>
#include <cstdio>
#include <stdexcept>

template <class V>
int mode_of (const V &v)
{
  if constexpr (requires { v.detection_mode (); })
    return int (v.detection_mode ());
  else
    return int (v.mode ());
}

void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("mode=%d\n", mode_of (v));
}

bool g (int x)
{
  if (x == 0)
    throw std::runtime_error ("throwing");
  return true;
}

void f (int x) pre (g (x)) { }

int main ()
{
  f (0);
  return 0;
}
