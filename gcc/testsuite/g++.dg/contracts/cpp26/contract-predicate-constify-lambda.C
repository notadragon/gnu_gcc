// [expr.prim.id.unqual]/3+d reaches inside a lambda that appears in the
// predicate.
//
// This is the paragraph's own example ([expr.prim.id.unqual]/3+e), which is
// where the rule is spelled out for the lambda case: a namespace-scope `int n`
// named inside `pre([=,&i,*this] mutable {...})` is const there, exactly as it
// would be named directly in the predicate.
//
// GCC used to constify only when the INNERMOST binding level was the contract
// scope (processing_contract_condition), which stops being true the moment a
// lambda in the predicate pushes its own scopes, so `++n` was accepted.  The
// captured parameter `++i` was rejected even then, but only incidentally: the
// capture initializer was constified out in the enclosing predicate, so the
// closure's member is already a reference to const.
//
// Everything the example marks OK must stay OK, and that is the substance of
// the test -- constifying too much here is as wrong as constifying too little.
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

int n = 0;

struct X
{
  bool m ();
};

struct Y
{
  int z = 0;

  void
  f (int i, int *p, int &r, X x, X *px)
      pre ([=, &i, *this] () mutable {
	++n;	     // { dg-error "increment of read-only location" }
	++i;	     // { dg-error "increment of read-only reference" }
	++p;	     // OK: a member of the closure type
	++r;	     // OK: a non-reference member of the closure type
	++this->z;   // OK: the captured *this
	++z;	     // OK: the captured *this
	(void) x;
	(void) px;

	int j = 17;  // declared INSIDE the predicate, so not constified
	++j;	     // OK

	[&] () {
	  int k = 34;
	  ++i;	     // { dg-error "increment of read-only reference" }
	  ++j;	     // OK
	  ++k;	     // OK
	} ();
	return true;
      } ())
  {
  }
};

// The storage durations, named from inside a lambda in the predicate rather
// than directly.  Each is "a variable declared outside of C".
int g_n = 0;
thread_local int t_n = 0;

struct HasStatic
{
  static int s;
};
int HasStatic::s = 0;

void
from_lambda ()
{
  static int local_static = 0;
  contract_assert ([] () {
    ++g_n;		 // { dg-error "increment of read-only location" }
    return true;
  } ());
  contract_assert ([] () {
    ++t_n;		 // { dg-error "increment of read-only location" }
    return true;
  } ());
  contract_assert ([] () {
    ++HasStatic::s;	 // { dg-error "increment of read-only location" }
    return true;
  } ());
  contract_assert ([&] () {
    ++local_static;	 // { dg-error "increment of read-only" }
    return true;
  } ());
}

// CONTROL: outside a contract entirely, a lambda constifies nothing.
void
not_in_a_contract ()
{
  [] () {
    ++n;
    ++g_n;
    ++t_n;
    ++HasStatic::s;
  } ();
}
