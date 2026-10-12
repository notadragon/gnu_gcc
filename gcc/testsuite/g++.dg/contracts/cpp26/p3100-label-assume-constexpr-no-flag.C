// P3100 x P3400 in constant evaluation, without -fcontracts-allow-assume: the
// configured "assume" is ignore, so a label allowing only assume leaves no
// valid evaluation semantic, a compute_semantic result of assume is not
// allowed, and a label that maps assume to enforce sees ignore (and gives
// observe).  The run-time shape is in p3100-label-no-flag.C.  (From the EDG
// fork's p3100/assume_labels_no_option.)
//
// Mirror: clang/test/Contracts/p3100-label-assume-constexpr-no-flag.cpp in
// the llvm_llvm-project fork, which words the first error "allows no
// evaluation semantics".
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3400 -fcontracts-p3100 -fcontract-evaluation-semantic=assume" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct only_assume_t {
  using assertion_control_object = only_assume_t;
  static constexpr evaluation_semantic_set allowed_semantics
    = {evaluation_semantic::assume};
};
struct only_observe_t {
  using assertion_control_object = only_observe_t;
  static constexpr evaluation_semantic_set allowed_semantics
    = {evaluation_semantic::observe};
};
struct ignore_enforce_t {
  using assertion_control_object = ignore_enforce_t;
  static constexpr evaluation_semantic_set allowed_semantics
    = {evaluation_semantic::ignore, evaluation_semantic::enforce};
};
struct compute_assume_t {
  using assertion_control_object = compute_assume_t;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic) const
  { return evaluation_semantic::assume; }
};
struct enforce_from_assume_t {
  using assertion_control_object = enforce_from_assume_t;
  constexpr evaluation_semantic compute_semantic (evaluation_semantic s) const
  {
    return s == evaluation_semantic::assume ? evaluation_semantic::enforce
					    : evaluation_semantic::observe;
  }
};

constexpr int a (int x) pre<only_assume_t{}> (x > 0) { return x; }  // { dg-error "no valid evaluation semantic" }
constexpr int b (int x) pre<only_observe_t{}> (x > 0) { return x; }  // { dg-warning "contract predicate is false in constant expression" }
constexpr int c (int x) pre<ignore_enforce_t{}> (x > 0) { return x; }
constexpr int d (int x) pre<compute_assume_t{}> (x > 0) { return x; }  // { dg-error "compute_semantic. result is not in the allowed" }
constexpr int e (int x) pre<enforce_from_assume_t{}> (x > 0) { return x; }  // { dg-warning "contract predicate is false in constant expression" }

constexpr int vb = b (0);  // observe: warns at the assertion
constexpr int vc = c (0);  // OK, ignore
constexpr int ve = e (0);  // observe (computed from ignore)

void use ()
{
  a (0);
  d (0);
}
