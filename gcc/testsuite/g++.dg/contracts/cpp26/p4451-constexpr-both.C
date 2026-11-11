// P4451: constant evaluation of a terminating contract assertion uses a
// terminating semantic too: with ignore configured, a false predicate makes
// the evaluation fail.  Under -fcontracts-p3400 and -fcontracts-p4451 (the P3400 label).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p4451 -fcontract-evaluation-semantic=ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

constexpr int pre_f (int x) pre<terminating> (x > 0) { return x; }	// { dg-error "contract predicate is false" }
constexpr int post_f (int x) post<terminating> (r : r > 0) { return x; }	// { dg-error "contract predicate is false" }
constexpr int assert_f (int x)
{
  contract_assert<terminating> (x > 0);	// { dg-error "contract predicate is false" }
  return x;
}
struct S
{
  static constexpr int mem (int x) pre<terminating> (x > 0) { return x; }	// { dg-error "contract predicate is false" }
};
template<typename T>
constexpr T tmpl (T x) pre<terminating> (x > 0) { return x; }	// { dg-error "contract predicate is false" }
constexpr auto lam = [] (int x) pre<terminating> (x > 0) { return x; };	// { dg-error "contract predicate is false" }

constexpr int a = pre_f (1);
constexpr int b = pre_f (-1);
constexpr int c = post_f (-1);
constexpr int d = assert_f (-1);
constexpr int e = S::mem (-1);
constexpr int f = tmpl (-1);
constexpr int g = lam (-1);
