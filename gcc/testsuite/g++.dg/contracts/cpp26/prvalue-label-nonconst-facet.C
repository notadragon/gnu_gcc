// The labels:: concepts probe a const object, so a non-const member function
// is not a facet, for a prvalue label `pre<L{}>' as for an lvalue one: only
// the global handler runs, with the original comment.
//
// Mirror: clang/test/Contracts/prvalue-label-nonconst-facet.cpp in
// the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe" }
// { dg-additional-options "-Wno-contract-invalid-label-facet" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

static int local_calls = 0, global_calls = 0;
static const char* comment;

struct L {
  using assertion_control_object = L;
  void handle_contract_violation (const std::contracts::contract_violation&)
  { ++local_calls; }
  constexpr const char* compute_comment (const char*) { return "nonconst"; }
};

static_assert (!std::contracts::labels::local_violation_label<L>);
static_assert (!std::contracts::labels::compute_comment_label<L>);

void handle_contract_violation (const std::contracts::contract_violation& v)
{
  ++global_calls;
  comment = v.comment ();
}

void f (int x) pre<L{}> (x > 0) {}

int main ()
{
  f (0);
  if (local_calls != 0 || global_calls != 1
      || !comment || std::strcmp (comment, "x > 0") != 0)
    __builtin_abort ();
}
