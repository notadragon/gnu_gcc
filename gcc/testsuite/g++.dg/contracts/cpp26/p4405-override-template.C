// D4405R0, "Virtual Functions": override shadows in templates, per
// instantiation of the overrider, with the overridden function's contract
// assertions as substituted for its own class -- a class template deriving
// from a dependent base, a non-template class deriving from a
// specialization, a class template deriving from a non-template class,
// and a destructor.  (A member function template cannot be virtual.)
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -fcontract-evaluation-semantic=observe -O2" }
// { dg-additional-options "-fdump-ipa-contract_shadow_clone" }

template <class T>
struct TB {
  virtual T f (T x) pre (x > T (0)) post (r: r > T (0));	// { dg-message "the predicate of the contract assertion" }
};

template <class T>
struct TD : TB<T> {
  T f (T x) override { return x; }
};

template <class T>
struct TE : TB<T> {
  T f (T x) override		// { dg-error "cannot be proven for a call through 'T TB<T>::f\\(T\\) \\\[with T = int\\\]'" }
  { return x - 1; }
};

struct ND : TB<int> {
  int f (int x) override	// { dg-error "cannot be proven for a call through 'T TB<T>::f\\(T\\) \\\[with T = int\\\]'" }
  { return -x; }
};

// The base's contract assertions depend on a non-type parameter.
template <int N>
struct NB {
  virtual int f (int x) pre (x > N) post (r: r > N);	// { dg-message "the predicate of the contract assertion" }
};

template <int N>
struct ND2 : NB<N> {
  int f (int x) override { return x; }
};

template <int N>
struct ND3 : NB<N> {
  int f (int x) override	// { dg-error "cannot be proven for a call through 'int NB<N>::f\\(int\\) \\\[with int N = 3\\\]'" }
  { return N; }
};

// A class template deriving from a non-template class.
struct B {
  virtual int f (int x) pre (x > 0) post (r: r > 0);
};

template <class T>
struct TF : B {
  int f (int x) override { return x + sizeof (T) - sizeof (T); }
};

// A destructor of a class template, implicitly defined.
int g;
template <class T>
struct DB {
  virtual ~DB () post (g == 1) { g = 1; }
};
template <class T>
struct DD : DB<T> { T t; };

TD<int> td;
TE<int> te;
TD<long> tdl;
ND nd;
ND2<3> nd2;
ND3<3> nd3;
TF<char> tf;
DD<int> dd;

// TD<int>, TE<int>, TD<long>, ND, ND2<3>, ND3<3>, TF<char>, DD<int>.
// { dg-final { scan-ipa-dump-times "Override shadow" 8 "contract_shadow_clone" } }
