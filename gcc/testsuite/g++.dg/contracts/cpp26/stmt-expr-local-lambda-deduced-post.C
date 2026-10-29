// A variable declared in a GNU statement expression in the postcondition of
// a lambda with a deduced return type, initialized from the result name.
// The predicate is parsed as if in a template while the return type is
// undeduced, and substituted once it is known.  start_decl handed the
// variable to push_template_decl, which with no template parameters failed
// silently, so the statement expression lost its declaration; then the
// variable's use, and an `auto' variable's substitution, needed the same
// allowance (GCC-632; stock GCC ICEs in the gimplifier instead).
//
// Mirror: clang/test/Contracts/stmt-expr-local-lambda-deduced-post.cpp in
// the llvm_llvm-project fork.
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

template <class T>
T
t (T v)
{
  auto l = [] (T x) post (r: __extension__ ({ T z = r; z > 0; })) { return x; };
  return l (v);
}

int
main ()
{
  auto l1 = [] (int x) post (r: __extension__ ({ int z = r; z > 0; }))
    { return x; };
  auto l2 = [] (int x) post (r: __extension__ ({ auto z = r; z > 0; }))
    { return x; };
  auto l3 = [] (int x)
    post (r: __extension__ ({ int z (r); int w{r}; z + w > 0; }))
    { return x; };
  auto l4 = [] (int x) post (r: __extension__ ({ int z = 1; z > r; }))
    { return x; };

  if (l1 (1) + l2 (1) + l3 (1) + l4 (0) != 3)
    __builtin_abort ();
  check (0);
  l1 (0);
  l2 (0);
  l3 (0);
  l4 (1);
  check (4);

  t (1);
  check (0);
  t (0);
  check (1);
}
