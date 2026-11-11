// A `query' member returning `const void*' is not the queryable facet
// (labels::queryable_label requires the result to be exactly `void*'), so it
// is ignored: the program compiles and query_control_object finds nothing.
//
// Mirror: clang/test/Contracts/query-const-void-return.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

#include <contracts>

struct L {
  using assertion_control_object = L;
  static constexpr int key = 1;
  const void* query (const void* k, std::size_t) const
  { return k == &key ? "found" : nullptr; }
};
static_assert (!std::contracts::labels::queryable_label<L>);
constexpr L lab{};

void handle_contract_violation (const std::contracts::contract_violation& v)
{
  if (v.query_control_object (&L::key))
    __builtin_abort ();
}

void f (int x) pre<lab> (x > 0) {}
