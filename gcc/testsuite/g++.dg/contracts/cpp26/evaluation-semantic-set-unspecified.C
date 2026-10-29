// evaluation_semantic::unspecified (0) can be put in an
// evaluation_semantic_set and asked about in constant evaluation (it once
// shifted by s - 1).
//
// Mirror: clang/test/Contracts/evaluation-semantic-set-unspecified.cpp.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

constexpr evaluation_semantic_set s (evaluation_semantic::unspecified);
static_assert (!evaluation_semantic_set::all ()
		.contains (evaluation_semantic::unspecified));
