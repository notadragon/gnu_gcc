// P3097: Verify that contracts on virtual functions are rejected without flag.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

struct Base {
  virtual void f() pre(true); // { dg-error "contracts cannot be added to virtual functions" }
};

struct Child : Base {
  void f() override pre(true); // { dg-error "contracts cannot be added to virtual functions" }
};
