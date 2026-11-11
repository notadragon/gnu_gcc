// A variable declared in a GNU statement expression in a lambda's
// precondition or postcondition is declared within the predicate, so each
// emission of the check must declare it too.  copy_contracts_list kept the
// variable rather than copying it, and remap_decls then left it out of the
// copied statement expression's BIND_EXPR, so the gimplifier met a variable
// no BIND_EXPR declared (gimplify_var_or_parm_decl, GCC-632; stock GCC too).
//
// Mirror: clang/test/Contracts/stmt-expr-local-lambda-pre.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

int violations;

void
handle_contract_violation (const std::contracts::contract_violation &)
{
  ++violations;
}

void
check (int n)
{
  if (violations != n)
    __builtin_abort ();
  violations = 0;
}

#ifndef NO_TEMPLATE
template <class T>
int
t (T v)
{
  auto l = [] (T x) pre (__extension__ ({ T y = x; y > 0; }))
    post (__extension__ ({ T y = 1; y > 0; })) { return 1; };
  return l (v);
}
#endif

int
main ()
{
  auto pre1 = [] (const int x) pre (__extension__ ({ int y = x; y > 0; })) {};
  pre1 (1);
  check (0);
  pre1 (0);
  check (1);

  // A declaration that does not use the parameter.
  auto pre2 = [] (int) pre (__extension__ ({ int y = 1; y > 0; })) {};
  pre2 (0);
  check (0);

  // Two in one predicate, and in a postcondition with an explicit return
  // type naming its result.
  auto post1 = [] (int x) -> int
    post (r: __extension__ ({ int z = r; int w (r); z + w > 0; }))
    { return x; };
  post1 (1);
  check (0);
  post1 (-1);
  check (1);

  // A capture beside the local.
  int k = 2;
  auto cap = [k] (int x) pre (__extension__ ({ int y = x; y < k; })) {};
  cap (1);
  check (0);
  cap (3);
  check (1);

#ifndef NO_TEMPLATE
  t (1);
  check (0);
  t (0);
  check (1);
#endif
}
