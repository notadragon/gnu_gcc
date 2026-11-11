// A contract predicate that exits via an exception during constant
// evaluation is a contract violation ([basic.contract.eval]); with a
// terminating semantic the enclosing manifestly constant-evaluated
// expression is ill-formed.  The exception does not escape the contract
// assertion, and its object is not reported as storage never deallocated.
// GCC-1006 (PR128543): the throw was never looked at, and the compiler
// segfaulted on the inline throw and reported the leaked exception object
// for a throwing call.
// { dg-do compile { target c++26 } }
// { dg-require-effective-target exceptions_enabled }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

#include <exception>

constexpr bool
thr ()
{
  throw 1;
  return true;
}

// Owns heap storage, freed only by its destructor.
struct owner
{
  int *p;
  constexpr owner () : p (new int (1)) {}
  constexpr owner (const owner &o) : p (new int (*o.p)) {}
  constexpr ~owner () { delete p; }
};

struct described : std::exception
{
  constexpr const char *what () const noexcept override { return "described"; }
};

constexpr bool
inline_throw ()
{
  contract_assert ((throw 1, true)); // { dg-error "contract predicate exited via an exception of type 'int' in constant expression" }
  return true;
}

constexpr bool
throwing_call ()
{
  contract_assert (thr ()); // { dg-error "contract predicate exited via an exception of type 'int' in constant expression" }
  return true;
}

constexpr bool
owning_exception ()
{
  contract_assert ((throw owner (), true)); // { dg-error "contract predicate exited via an exception of type 'owner' in constant expression" }
  return true;
}

constexpr bool
what_exception ()
{
  contract_assert ((throw described (), true)); // { dg-error "contract predicate exited via an exception of type 'described' in constant expression; 'what\\(\\)': 'described'" }
  return true;
}

constexpr bool
pre_throw (int x)
  pre ((throw x, true)) // { dg-error "contract predicate exited via an exception of type 'int' in constant expression" }
{
  return true;
}

constexpr bool
post_throw (const int x)
  post ((throw x, true)) // { dg-error "contract predicate exited via an exception of type 'int' in constant expression" }
{
  return true;
}

static_assert (inline_throw ());
static_assert (throwing_call ());
static_assert (owning_exception ());
static_assert (what_exception ());
static_assert (pre_throw (1));
static_assert (post_throw (1));

// CONTROL: an exception the predicate catches itself does not exit it.
constexpr bool
caught_inside ()
{
  contract_assert ([] { try { throw 1; } catch (int) { return true; } } ());
  return true;
}

static_assert (caught_inside ());

// Not manifestly constant-evaluated: no diagnostic, the violation is left
// to run time.
bool
at_run_time ()
{
  return inline_throw ();
}
