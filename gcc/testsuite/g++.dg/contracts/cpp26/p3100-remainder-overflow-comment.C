// The P3100 checks on `%' name the operation consistently: remainder by zero
// and INT_MIN % -1 use the same word.
//
// Mirror: clang/test/Contracts/p3100-remainder-overflow-comment.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3100" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p3100-remainder-overflow-comment.json" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

static const char *last = "";
void handle_contract_violation (const std::contracts::contract_violation& v)
{ last = v.comment (); }

[[gnu::noinline]] int remi (int a, int b) { return a % b; }

int main () {
  volatile int zero = 0;
  remi (1, zero);
  bool zero_says_rem = std::strstr (last, "remainder") != nullptr;
  last = "";
  remi (-__INT_MAX__ - 1, -1);
  bool ovf_says_rem = std::strstr (last, "remainder") != nullptr;
  if (*last == '\0' || zero_says_rem != ovf_says_rem)
    __builtin_abort ();
}
