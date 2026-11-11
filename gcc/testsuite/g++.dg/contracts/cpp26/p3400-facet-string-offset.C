// P3400: a compute_comment / compute_message facet receives a pointer into a
// real string literal and may return a pointer into it at an offset; the
// string is read through the returned pointer, offset included.
//
// Clang: clang/test/Contracts/p3400-facet-string-offset.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p3099 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

static const char *comment, *message;

void handle_contract_violation (const std::contracts::contract_violation& v)
{
  comment = v.comment ();
  message = v.message ();
}

struct skip1_t {
  using assertion_control_object = skip1_t;
  constexpr const char* compute_comment (const char* c) const
  { return c ? c + 1 : c; }
  constexpr const char* compute_message (const char* m) const
  { return m ? m + 1 : m; }
};
constexpr skip1_t skip1{};

void f (int x) pre<skip1> (x > 0, "xmsg") {}

int main ()
{
  f (0);
  if (!comment || std::strcmp (comment, " > 0") != 0
      || !message || std::strcmp (message, "msg") != 0)
    __builtin_abort ();
}
