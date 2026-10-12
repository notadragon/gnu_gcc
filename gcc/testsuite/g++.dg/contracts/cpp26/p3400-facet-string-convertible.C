// P3400: a compute_comment / compute_message facet whose result is a class
// type convertible to const char* is applied: the result is converted as the
// concept requires.
//
// Clang: clang/test/Contracts/p3400-facet-string-convertible.cpp in the
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

struct cstr {
  const char* p;
  constexpr operator const char* () const { return p; }
};
struct conv_t {
  using assertion_control_object = conv_t;
  constexpr cstr compute_comment (const char*) const { return {"conv"}; }
  constexpr cstr compute_message (const char*) const { return {"conv"}; }
};
constexpr conv_t conv{};

void f (int x) pre<conv> (x > 0, "m") {}

int main ()
{
  f (0);
  if (!comment || std::strcmp (comment, "conv") != 0
      || !message || std::strcmp (message, "conv") != 0)
    __builtin_abort ();
}
