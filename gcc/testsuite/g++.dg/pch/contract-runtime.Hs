// { dg-options "-std=c++26 -fcontracts -fcontract-evaluation-semantic=quick_enforce" }
int f (int x) pre (x > 0) post (r: r > 1) { return x; }
inline void g (int x) { contract_assert (x != 3); }
struct M { int m (int x) pre (x > 0) { return x; } };
inline auto lam = [] (int x) pre (x > 0) { return x; };
template <class T> T t0 (T x) pre (x > 0) post (r: r > 0) { return x; }
template <class T> T t1 (T x) pre (x > 1) post (r: r > 1) { return x; }
template <class T> T t2 (T x) pre (x > 2) post (r: r > 2) { return x; }
template <class T> T t3 (T x) pre (x > 3) post (r: r > 3) { return x; }
template <class T> T t4 (T x) pre (x > 4) post (r: r > 4) { return x; }
template <class T> T t5 (T x) pre (x > 5) post (r: r > 5) { return x; }
template <class T> T t6 (T x) pre (x > 6) post (r: r > 6) { return x; }
template <class T> T t7 (T x) pre (x > 7) post (r: r > 7) { return x; }
