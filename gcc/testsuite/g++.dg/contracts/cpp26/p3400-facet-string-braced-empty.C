// P3400: a compute_comment / compute_message facet returning a braced array,
// or "" (a redacting facet), is applied; an empty result is a result.
//
// Clang: clang/test/Contracts/p3400-facet-string-braced.cpp in the
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

struct braced_t {
  using assertion_control_object = braced_t;
  static constexpr char txt[] = {'b', 'r', 'a', 'c', 'e', 'd', '\0'};
  constexpr const char* compute_comment (const char*) const { return txt; }
  constexpr const char* compute_message (const char*) const { return txt; }
};
constexpr braced_t braced{};

struct empty_t {
  using assertion_control_object = empty_t;
  constexpr const char* compute_comment (const char*) const { return ""; }
  constexpr const char* compute_message (const char*) const { return ""; }
};
constexpr empty_t empty{};

void fb (int x) pre<braced> (x > 0, "m") {}
void fe (int x) pre<empty> (x > 0, "m") {}

int main ()
{
  fb (0);
  if (!comment || std::strcmp (comment, "braced") != 0
      || !message || std::strcmp (message, "braced") != 0)
    __builtin_abort ();
  fe (0);
  if (!comment || std::strcmp (comment, "") != 0
      || !message || std::strcmp (message, "") != 0)
    __builtin_abort ();
}
