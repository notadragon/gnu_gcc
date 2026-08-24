/* Test that one label type yields one facet trampoline no matter how many
   template instantiations mention it.

   Regression test: the trampoline caches are keyed by the label's type
   tree.  Substituting a dependent label can produce a distinct cv-variant
   tree per instantiation for one and the same type, so the cache missed and
   a fresh trampoline was emitted for every instantiation.  Keying on
   TYPE_MAIN_VARIANT collapses them back to one.

   The count is checked with a dg-final scan rather than at run time because
   the duplication was pure code bloat -- the names are counter-unique and
   internal-linkage, so the program behaved correctly either way, which is
   exactly why this went unnoticed.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe -fdump-tree-optimized" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::violation_handled;

int calls = 0;

struct one_t
{
  using assertion_control_object = one_t;
  violation_handled
  handle_contract_violation (const contract_violation &) const
  { ++calls; return violation_handled::handled; }
};

/* Spelled so that each instantiation substitutes to its own type tree for
   what is nonetheless always one_t.  */
template <class T> struct wrap { using type = one_t; };

template <class T> constexpr typename wrap<T>::type dependent_label{};

template <class T>
void g (T x) pre<dependent_label<T>> (x > 0) { }

int
main ()
{
  g<int> (-1);
  g<long> (-1);
  g<char> (-1);
  g<short> (-1);

  if (calls != 4)
    __builtin_abort ();
}

/* One trampoline for the one label type, not one per instantiation.  The
   trampoline counter is per-TU and this TU has a single label type, so a
   second trampoline appearing at all is the defect: before the fix all
   four instantiations got their own (_0 through _3).  */
// { dg-final { scan-tree-dump "__contract_local_handler_0" "optimized" } }
// { dg-final { scan-tree-dump-not "__contract_local_handler_1" "optimized" } }
