// { dg-options "-fcontracts-p3850" }
// { dg-do compile { target c++26 } }

// P4405: the prove and diagnose enumerators of evaluation_semantic (8 and 9,
// the next values after the P4298 extensions), their place in
// evaluation_semantic_set's all() and ~, and their classification: like
// ignore, neither is checked nor terminating, both are continuing, and
// neither lets an exception out of handling.  The enumerators are
// unconditional, as assume's is.
//
// Mirror: libcxx/test/std/contracts/p4405_semantics.compile.pass.cpp in the
// llvm_llvm-project fork.

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using std::contracts::is_checked;
using std::contracts::is_unchecked;
using std::contracts::is_terminating;
using std::contracts::is_continuing;
using std::contracts::is_nonthrowing;
using enum evaluation_semantic;

static_assert(static_cast<int>(prove) == 8);
static_assert(static_cast<int>(diagnose) == 9);

static_assert(evaluation_semantic_set::all().contains(prove));
static_assert(evaluation_semantic_set::all().contains(diagnose));
static_assert((~evaluation_semantic_set{prove})
	      == (evaluation_semantic_set::all()
		  & ~evaluation_semantic_set{prove}));
static_assert(!(~evaluation_semantic_set{prove}).contains(prove));
static_assert((~evaluation_semantic_set{prove}).contains(diagnose));
static_assert((~evaluation_semantic_set::all()).empty());
static_assert(evaluation_semantic_set{prove, diagnose}.size() == 2);

static_assert(!is_checked(prove));
static_assert(!is_checked(diagnose));
static_assert(is_unchecked(prove));
static_assert(is_unchecked(diagnose));
static_assert(!is_terminating(prove));
static_assert(!is_terminating(diagnose));
static_assert(is_continuing(prove));
static_assert(is_continuing(diagnose));
static_assert(is_nonthrowing(prove));
static_assert(is_nonthrowing(diagnose));
