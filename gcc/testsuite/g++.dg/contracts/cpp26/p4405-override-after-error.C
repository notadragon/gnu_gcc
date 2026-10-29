// D4405R0, "Virtual Functions", after an error elsewhere in the translation
// unit (GCC-947).  The middle end then does not run: no proof shadow and no
// override shadow is made, and every obligation left to one is diagnosed as
// unproven, as at the end of any compilation that stopped at an error.  An
// override pair's obligations are diagnosed in the same way -- they used to
// be dropped -- and so are those folding decided at once; DECISIONS.md E150
// still holds: an inline overrider whose vtable another translation unit
// emits is not diagnosed here, nor is one of a class nothing here
// constructs.  Mirror: edgcpp_compiler
// tests/tests/contracts/p4405override/override_other_error_cgen.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2" }

int bad = undeclared;		// { dg-error "not declared" }

struct B {
  virtual int f (int x)
    pre (x > 0)
    post (r: r > 0);		// { dg-message "the predicate of the contract assertion" }
};

// Its precondition and B::f's postcondition, proven in its override shadow
// without the error, unproven.
struct D : B {
  int f (int x) override	// { dg-error "cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
    pre (x > 0)			// { dg-message "the predicate of the contract assertion" }
  {
    return x;
  }
};

// Its precondition decided at once by folding; B::f's postcondition, proven
// in its override shadow without the error, unproven.
struct F : B {
  int f (int x) override	// { dg-error "always false for a call through 'virtual int B::f\\(int\\)'" }
  // { dg-error "cannot be proven for a call through 'virtual int B::f\\(int\\)'" "" { target *-*-* } .-1 }
    pre (false)			// { dg-message "the predicate of the contract assertion" }
  {
    return 1;
  }
};

// Its vtable is emitted elsewhere (the key function is defined in another
// translation unit): not diagnosed here.
struct Elsewhere : B {
  virtual void key ();
  int f (int x) override	// { dg-bogus "for a call through" }
    pre (false)
  {
    return 1;
  }
};

// Its vtable is emitted here (the key function is defined here): both.
struct KeyHere : B {
  virtual void key ();
  int f (int x) override	// { dg-error "cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
    pre (x > 1)			// { dg-message "the predicate of the contract assertion" }
  {
    return 1;
  }
};
void KeyHere::key () {}

// Not inline: both, at its definition, wherever its vtable is emitted.
struct OutOfLine : B {
  virtual void key ();
  int f (int x) override
    pre (x > 2);		// { dg-message "the predicate of the contract assertion" }
};
int OutOfLine::f (int x)	// { dg-error "cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
{
  return x;
}

// Nothing constructs it, so nothing emits its vtable: not diagnosed.
struct Unused : B {
  int f (int x) override	// { dg-bogus "for a call through" }
    pre (x > 3)
  {
    return x;
  }
};

// A class template's instance, constructed here: both.
template <typename T>
struct TI : B {
  int f (int x) override	// { dg-error "cannot be proven for a call through 'virtual int B::f\\(int\\)'" }
    pre (x > 4)			// { dg-message "the predicate of the contract assertion" }
  {
    return x;
  }
};
TI<int> ti;

D d;
F fo;
