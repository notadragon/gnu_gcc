// P3097: Virtual functions accept contracts; overrides are independent (no
// contract inheritance).  Sema-only acceptance across hierarchy shapes.
// (Port of Clang clang/test/Contracts/p3097-basic-sema.cpp)
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3097" }

struct Base {
  virtual int f(int x)
    pre(x > 0)
    post(r: r > 0);
  virtual void g() = 0;
};

struct Derived : Base {
  int f(int x) override
    pre(x > -100)
    post(r: r > -100);
  void g() override;
};

int Base::f(int x) { return x; }
int Derived::f(int x) { return x; }
void Derived::g() { }

struct Abstract {
  virtual void h(int x) pre(x > 0) = 0;
  virtual int k(int x) pre(x > 0) post(r: r >= 0) = 0;
};

struct Concrete : Abstract {
  void h(int x) override pre(x > -100) { }
  int k(int x) override pre(x > -50) post(r: r >= -50) { return x; }
};

struct NoContracts : Base {
  int f(int x) override { return x; }
  void g() override { }
};

struct X { virtual void m(int n) pre(n > 0) { } };
struct Y : X { void m(int n) override pre(n > -10) { } };
struct Z : Y { void m(int n) override pre(n > -100) { } };

struct A1 { virtual void v(int n) pre(n > 0) { } };
struct A2 { virtual void v(int n) pre(n > 10) { } };
struct B : A1, A2 { void v(int n) override pre(n > -100) { } };

struct VBase { virtual void w(int x) pre(x > 0) { } };
struct Mid1 : virtual VBase { void w(int x) override pre(x > -10) { } };
struct Mid2 : virtual VBase { void w(int x) override pre(x > -20) { } };
struct Bottom : Mid1, Mid2 { void w(int x) override pre(x > -100) { } };
