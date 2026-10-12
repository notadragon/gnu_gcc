// P3400: `compute_message (const char* m) const { return m + 1; }' is a
// constant expression on the message it is given: the facet is applied to the
// parsed message, not to the null placeholder a deferred contract carries
// until then.
//
// Clang: clang/test/Contracts/p3400-facet-string-offset.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p3099 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

#include <contracts>

struct skip1_t {
  using assertion_control_object = skip1_t;
  constexpr const char* compute_message (const char* m) const { return m + 1; }
};
constexpr skip1_t skip1{};

void f (int x) pre<skip1> (x > 0, "xmsg") {}
