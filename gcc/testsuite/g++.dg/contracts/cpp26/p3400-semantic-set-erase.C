// P3400 [support.contract.control.semset.mod] names the member that removes
// one evaluation semantic from an evaluation_semantic_set erase, as std::set
// does (GCC-900).  The earlier name, remove, is not kept as an alias.
//
// Mirror: llvm_llvm-project clang/test/Contracts/p3400-semantic-set-erase.cpp.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400" }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

template<typename _Set>
  concept has_erase
    = requires (_Set s) { s.erase (evaluation_semantic::observe); };

template<typename _Set>
  concept has_remove
    = requires (_Set s) { s.remove (evaluation_semantic::observe); };

// Control: the same form of check finds a member that exists.
template<typename _Set>
  concept has_insert
    = requires (_Set s) { s.insert (evaluation_semantic::observe); };

static_assert (has_insert<evaluation_semantic_set>);
static_assert (has_erase<evaluation_semantic_set>);
static_assert (!has_remove<evaluation_semantic_set>);

constexpr bool
erase_one ()
{
  evaluation_semantic_set s{evaluation_semantic::observe,
			    evaluation_semantic::enforce};
  s.erase (evaluation_semantic::observe);
  // Erasing a semantic the set does not contain changes nothing.
  s.erase (evaluation_semantic::ignore);
  return !s.contains (evaluation_semantic::observe)
	 && s.contains (evaluation_semantic::enforce)
	 && s.size () == 1;
}
static_assert (erase_one ());
