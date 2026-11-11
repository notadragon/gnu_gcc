// A contract predicate that exits via an exception during constant
// evaluation is a contract violation ([basic.contract.eval]); with the
// observe semantic it is diagnosed and the evaluation continues after the
// assertion, so the enclosing expression stays constant.  The exception does
// not escape the contract assertion, and its object -- and the storage it
// owns -- is not reported as never deallocated.  GCC-1006 (PR128543).
// { dg-do compile { target c++26 } }
// { dg-require-effective-target exceptions_enabled }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }

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

constexpr int
f ()
{
  int n = 0;
  contract_assert ((throw 1, true)); // { dg-warning "contract predicate exited via an exception of type 'int' in constant expression" }
  ++n;
  contract_assert (thr ()); // { dg-warning "contract predicate exited via an exception of type 'int' in constant expression" }
  ++n;
  contract_assert ((throw owner (), true)); // { dg-warning "contract predicate exited via an exception of type 'owner' in constant expression" }
  ++n;
  return n;
}

static_assert (f () == 3);
