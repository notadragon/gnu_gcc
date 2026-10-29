// D4405R0, "Virtual Functions", DECISIONS.md E152: a constexpr virtual
// destructor, user-provided or implicitly defined, gets its override
// shadows like any other, and constant evaluation of it is unaffected: the
// override regions, and the try/finally that holds the overridden
// destructor's postcondition's obligation, are never executed there.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -fcontract-evaluation-semantic=observe -O2" }

int g;

struct CB {
  constexpr virtual ~CB ()
    post (__builtin_is_constant_evaluated () || g == 1)	// { dg-error "contract assertion cannot be proven" }
  {}	// { dg-message "the predicate of the contract assertion" "" { target *-*-* } .-1 }
};

struct CD : CB {
  constexpr ~CD () override	// { dg-bogus "cannot be proven" }
  { if !consteval { g = 1; } }
};

struct CE : CB {		// { dg-error "cannot be proven for a call through '.*CB::~CB\\(\\)'" }
  int i = 0;
};

constexpr bool
destroy ()
{
  CB *p = new CD;
  delete p;
  p = new CE;
  delete p;
  { CD d; CE e; }
  return true;
}

static_assert (destroy ());

void use () { CD d; CE e; }
