// Watch test for an open bug (GCC-683, stock GCC too, PR65174; not a
// contracts bug): a delete-expression invokes the destructor and the
// deallocation function, so it is potentially-throwing when the destructor
// is ([except.spec]/6), whatever the value of its operand.  delete_sanity
// replaces the delete of a null pointer constant with a no-op before the
// destructor call is built, so noexcept says true for it (and the
// destructor's access is never checked there).  A null pointer that is not
// a literal constant is fine.  When fixed, the xfailed dg-bogus lines
// report an XPASS.
//
// It bears on contracts because a predicate that can throw is evaluated
// under the evaluation_exception handling (predicate-throw-delete.C).
//
// Mirror: clang/test/Contracts/noexcept-delete-null.cpp in the
// llvm_llvm-project fork, where Clang passes.
// { dg-do compile { target c++11 } }

struct D { ~D () noexcept (false); };
D *p;
constexpr D *np = nullptr;

static_assert (!noexcept (delete p), "");
static_assert (!noexcept (delete np), "");
static_assert (!noexcept (delete (D *) 0), "");		// { dg-bogus "static assertion failed" "GCC-683" { xfail *-*-* } }
static_assert (!noexcept (delete static_cast<D *> (nullptr)), "");	// { dg-bogus "static assertion failed" "GCC-683" { xfail *-*-* } }
static_assert (!noexcept (delete[] (D *) 0), "");	// { dg-bogus "static assertion failed" "GCC-683" { xfail *-*-* } }
