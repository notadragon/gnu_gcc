// P3099: a contract diagnostic message preserves escape sequences and handles a
// long (multi-fragment) string literal; the message reaches the handler verbatim.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts-p3099 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstring>

static const char* last = nullptr;
void handle_contract_violation(const std::contracts::contract_violation& v) {
  last = v.message();
}

void f(int x) pre(x > 0, "line1\nline2\ttab \"quoted\" backslash\\end") { }

void g(int x) pre(x > 0,
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
  "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb") { }

int main() {
  f(-1);
  if (!last
      || std::strcmp(last, "line1\nline2\ttab \"quoted\" backslash\\end") != 0)
    __builtin_abort();
  g(-1);
  if (!last || std::strlen(last) != 152) __builtin_abort();
}
