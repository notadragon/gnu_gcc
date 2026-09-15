// P3097: a redeclaration of the SAME virtual function must repeat matching
// contracts; a mismatch is an error.  (An override is a different function and
// is independent -- no contract inheritance.)
// (Port of Clang clang/test/Contracts/p3097-override-sema.cpp)
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3097" }

struct Base {
  virtual int f(int x) pre(x > 0) post(r: r > 0);
};

// Redeclaration (out-of-line definition) with matching contracts -- OK.
int Base::f(int x) pre(x > 0) post(r: r > 0) { return x; }

struct Bad {
  virtual int g(int x) pre(x > 0); // { dg-note "previous contract here" }
};

// Mismatched contract on the out-of-line definition -- error.
int Bad::g(int x) pre(x > 10) { return x; } // { dg-error "mismatched contract condition in declaration" }
