// P3400: -Wno-contract-invalid-label-facet turns the near-miss warning off.
//
// The warning is on by default, so a codebase that deliberately keeps a
// name-sharing private helper the front end cannot tell apart from a mistake
// needs a way to silence it.  Same shapes as p3400-facet-near-miss.C; nothing
// here may produce a diagnostic.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -Wno-contract-invalid-label-facet" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <cstddef>

using std::contracts::contract_violation;
using std::contracts::evaluation_semantic;

struct priv_handler_t {
  using assertion_control_object = priv_handler_t;
private:
  void handle_contract_violation (const contract_violation&) const { }
};
constexpr priv_handler_t priv_handler{};

struct mut_handler_t {
  using assertion_control_object = mut_handler_t;
  void handle_contract_violation (const contract_violation&) { }
};
constexpr mut_handler_t mut_handler{};

struct mut_comment_t {
  using assertion_control_object = mut_comment_t;
  constexpr const char* compute_comment (const char*) { return "x"; }
};
constexpr mut_comment_t mut_comment{};

struct priv_query_t {
  using assertion_control_object = priv_query_t;
private:
  void* query (const void*, std::size_t) const { return nullptr; }
};
constexpr priv_query_t priv_query{};

void f1 (int x) pre<priv_handler> (x > 0) { }
void f2 (int x) pre<mut_handler>  (x > 0) { }
void f3 (int x) pre<mut_comment>  (x > 0) { }
void f4 (int x) pre<priv_query>   (x > 0) { }
