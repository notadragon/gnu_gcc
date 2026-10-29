// { dg-do compile { target c++23 } }
// { dg-additional-options "-fcontracts" }

void gfn3(int n) pre ( n > 0 );

struct Outer {
  struct Inner {
    void fn(int n) pre ( n > 0 && bob > 1 );
    void fn2(int n) pre ( n > 0 && bob > 1 );
  };

  void fn(int m) pre (m > 1 );

  // A contract-assertion is a complete-class context, so `bob' is visible
  // here just as it is on Inner::fn's own declaration above -- the same
  // predicate text cannot mean two different things in the two places.
  // This used to be diagnosed as `bob' not being declared: the specifier
  // was parsed at whatever parameter list preceded it, in a scope that had
  // not yet seen Outer's members.  Clang accepts it, and diagnoses a
  // genuinely undeclared name here, which is what fn3 below pins.
  friend void Inner::fn(int n) pre ( n > 0 && bob > 1 );

  // Two friend declarations of one function with different predicates.  The
  // parameters are spelled differently, so the comparison binds them
  // positionally: `q > 1' against `p > 0' is a mismatch.  This went
  // undiagnosed while a deferred contract was never re-compared after being
  // parsed (CC-1 / F31).
  friend void gfn(int p) pre ( p > 0 );
  friend void gfn(int q) pre ( q > 1 ); // { dg-error "mismatched contract condition" }

  friend void gfn2(int q);
  friend void gfn2(int p) pre ( p > 0 ) { } // { dg-error "declaration adds contracts" }

  // A qualified friend declaration redeclares a member that already exists,
  // so it never reaches duplicate_decls -- check_classfn returns the member
  // and the friend declaration is dropped.  Its contracts went with it: the
  // predicate below was accepted however it was written.  These two pin that
  // it is now both parsed and compared.
  friend void Inner::fn2(int n) pre ( n > 0 && nope > 1 ); // { dg-error "not declared" }
  friend void Inner::fn(int n) pre ( n < 0 ); // { dg-error "mismatched contract" }

  static int bob;
};
int Outer::bob{-1};
