// P3400: a combined label runs its components' local violation handlers
// right to left -- control flows back from the predicate towards the global
// handler, which comes last -- and one that returns handled stops the chain
// (GCC-628: they ran left to right).
// (Clang mirror: clang/test/Contracts/Runnable/p3400-local-handler-chain-order.cpp)
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstdio>

using namespace std::contracts;
using namespace std::contracts::labels;

void handle_contract_violation (const contract_violation &)
{
  std::printf (" global");
}

struct l1_t
{
  using assertion_control_object = l1_t;
  violation_handled handle_contract_violation (const contract_violation &) const
  {
    std::printf (" l1");
    return violation_handled::not_handled;
  }
};
struct l2_t
{
  using assertion_control_object = l2_t;
  void handle_contract_violation (const contract_violation &) const
  {
    std::printf (" l2");
  }
};
struct h_t
{
  using assertion_control_object = h_t;
  static violation_handled handle_contract_violation (const contract_violation &)
  {
    std::printf (" h");
    return violation_handled::handled;
  }
};
constexpr l1_t l1{};
constexpr l2_t l2{};
constexpr h_t h{};

void a (int x) pre<l1 | l2> (x > 0) {}
void b (int x) pre<l1 | h> (x > 0) {}
void c (int x) pre<h | l1> (x > 0) {}
void d (int x) pre<l1 | l2 | h> (x > 0) {}
void e (int x) pre<h | l1 | l2> (x > 0) {}

int
main ()
{
  std::printf ("a:"); a (-1); std::printf ("\n");
  std::printf ("b:"); b (-1); std::printf ("\n");
  std::printf ("c:"); c (-1); std::printf ("\n");
  std::printf ("d:"); d (-1); std::printf ("\n");
  std::printf ("e:"); e (-1); std::printf ("\n");
}

// { dg-output "a: l2 l1 global(\n|\r\n|\r)" }
// { dg-output "b: h(\n|\r\n|\r)" }
// { dg-output "c: l1 h(\n|\r\n|\r)" }
// { dg-output "d: h(\n|\r\n|\r)" }
// { dg-output "e: l2 l1 h(\n|\r\n|\r)" }
