// D4405R0, "Virtual Functions": override shadows change nothing in the
// object code, and neither do the obligations an overrider takes on
// without them (-fno-contracts-prove-override: B::f's postconditions,
// with the snapshots and conversions they read).  A translation unit whose
// overriders -- with and without contract assertions of their own
// (preconditions and postconditions), through a primary, a non-primary
// and a virtual base, with a covariant result, a modified parameter, a
// loop, a throwing body, a class returned by the named return value, and
// destructors, user-provided and implicitly defined, of classes with and
// without virtual bases, with a throwing member destructor and a
// precondition of their own -- are analyzed with diagnose is compiled to
// the same object code as with every analysis evaluation ignore: with and
// without -fcontracts-prove-override and -fcontracts-prove-assume, with
// the runtime evaluation semantics ignore and observe as well as enforce,
// and with -fcontract-checks-outlined.  contracts-same-object-code
// (lib/contracts-objects.exp) compares the objects' content, not the order
// of their sections, which can differ (the order in which comdat functions
// are finished).
// Mirror: clang/test/Contracts/p4405-override-same-code.cpp
// { dg-do assemble { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -O2" }
int sink (int);

struct Pad { int pad[4]; virtual void p (); };

struct B {
  int m;
  virtual int f (int x) pre (x > 0) post (r: r > 0);
  virtual B *self () post (r: r == this);
  virtual int get (const int x) pre (x > 0) post (r: r >= x);
  virtual int loop (int n) pre (n >= 0) post (r: r >= 0);
};

struct V { int v; virtual int g (int x) pre (x > v); };

struct D : Pad, B, virtual V {
  int f (int x) override pre (x >= 0) { return x >= 1 ? x : sink (x); }
  D *self () override { return this; }
  int get (int x) override { int r = x; x = sink (x); return r + (x > 0); }
  int loop (int n) override
  {
    int s = 0;
    for (int i = 0; i < n; ++i)
      s += sink (i) & 1;
    return s;
  }
  int g (int x) override pre (x > v + 1) { return x - v; }
};

struct E : D {
  int f (int x) override pre (x > 5) { return x; }
};

struct R { R (); R (const R &); ~R (); int v; };

struct P {
  virtual int t (int x) pre (x > 0) post (r: r > 0);
  virtual R make (int x) pre (x > 0) post (r: r.v > 0);
  virtual int own (const int x) pre (x > 0) post (r: r > 0);
  virtual ~P () noexcept (false) post (sink (0) == 0);
};

struct M { ~M () noexcept (false); int v; };

struct Q : P {
  int t (int x) override { if (x > 100) throw 1; return sink (x); }
  R make (int x) override { R r; r.v = sink (x); return r; }
  int own (const int x) override post (r: r >= x) { return x + sink (x); }
  ~Q () override pre (sink (1) > 0) { sink (2); }
};

struct Implicit : P { M m; };
struct VDtor : virtual P { int i; };
struct VDtor2 : VDtor {
  ~VDtor2 () override { for (int i = 0; i < sink (3); ++i) sink (i); }
};

void kill (P *p) { delete p; }

void use () { D d; E e; Q q; Implicit i; VDtor v; VDtor2 v2; }

// { dg-final { contracts-same-object-code "-fcontract-analysis-semantic=ignore" "" "-fno-contracts-prove-override" "-fno-contracts-prove-assume" "-fno-contracts-prove-override -fno-contracts-prove-assume" } }
// { dg-final { contracts-same-object-code "-fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=ignore" "-fcontract-evaluation-semantic=ignore" "-fcontract-evaluation-semantic=ignore -fno-contracts-prove-override" } }
// { dg-final { contracts-same-object-code "-fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=ignore" "-fcontract-evaluation-semantic=observe" "-fcontract-evaluation-semantic=observe -fno-contracts-prove-override" } }
// { dg-final { contracts-same-object-code "-fcontract-checks-outlined -fcontract-analysis-semantic=ignore" "-fcontract-checks-outlined" "-fcontract-checks-outlined -fno-contracts-prove-override" } }
