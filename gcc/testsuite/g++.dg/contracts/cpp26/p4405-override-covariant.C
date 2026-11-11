// D4405R0, "Virtual Functions": with a covariant return type, the
// overridden function's postconditions see the overrider's result
// converted to the overridden function's return type, as the covariant
// thunk converts it -- null-checked for a pointer -- and `this' is the
// base class subobject.  B is not D's primary base, so both conversions
// adjust the pointer.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -O2" }

struct X { int pad; virtual void x (); };

struct B {
  virtual B *self () post (r: r == this);
  virtual B &ref () post (r: &r == this);
  virtual B *maybe (bool b) post (r: r != nullptr);	// { dg-message "the predicate of the contract assertion" }
  virtual B *never () post (r: r == nullptr);
};

struct D : X, B {
  D *self () override { return this; }		// { dg-bogus "cannot be proven" }
  D &ref () override { return *this; }		// { dg-bogus "cannot be proven" }
  // Not proven: a null result stays null in the conversion.
  D *maybe (bool b) override			// { dg-error "cannot be proven for a call through 'virtual B\\* B::maybe\\(bool\\)'" }
  { return b ? this : nullptr; }
  D *never () override { return nullptr; }	// { dg-bogus "cannot be proven" }
};

void use () { D d; }
