// The assertion_kind enumerators of unadopted papers are declared only
// where the paper is enabled, as in libc++ (notadragon_wg21 DECISIONS.md
// C9): manual and cassert with P3290's library API, post_capture with
// P3098's postcondition captures.  Here neither is enabled.
// assertion-kind-gated-enabled.C enables both.
//
// Mirror: libc++'s <contracts> in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }
// { dg-skip-if "requires hosted libstdc++" { ! hostedlib } }

#include <contracts>

using std::contracts::assertion_kind;

template <class E> concept has_manual = requires { E::manual; };
template <class E> concept has_cassert = requires { E::cassert; };
template <class E> concept has_post_capture = requires { E::post_capture; };
template <class E> concept has_implicit = requires { E::implicit; };

static_assert (!has_manual<assertion_kind>);
static_assert (!has_cassert<assertion_kind>);
static_assert (!has_post_capture<assertion_kind>);
static_assert (has_implicit<assertion_kind>);
static_assert (static_cast<int> (assertion_kind::assert) == 3);
