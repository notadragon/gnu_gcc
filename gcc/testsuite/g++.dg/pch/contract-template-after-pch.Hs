// { dg-options "-std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce" }
template <class T> constexpr T t0 (T x)
  pre (x > 0)
  post (r: r < 9) { return x; }
template <class T> constexpr T t1 (T x)
  pre (x > 1)
  post (r: r < 10) { return x; }
template <class T> constexpr T t2 (T x)
  pre (x > 2)
  post (r: r < 11) { return x; }
template <class T> constexpr T t3 (T x)
  pre (x > 3)
  post (r: r < 12) { return x; }
template <class T> constexpr T t4 (T x)
  pre (x > 4)
  post (r: r < 13) { return x; }
template <class T> constexpr T t5 (T x)
  pre (x > 5)
  post (r: r < 14) { return x; }
template <class T> constexpr T t6 (T x)
  pre (x > 6)
  post (r: r < 15) { return x; }
template <class T> constexpr T t7 (T x)
  pre (x > 7)
  post (r: r < 16) { return x; }
template <class T> constexpr T t8 (T x)
  pre (x > 8)
  post (r: r < 17) { return x; }
template <class T> constexpr T t9 (T x)
  pre (x > 9)
  post (r: r < 18) { return x; }
template <class T> constexpr T t10 (T x)
  pre (x > 10)
  post (r: r < 19) { return x; }
template <class T> constexpr T t11 (T x)
  pre (x > 11)
  post (r: r < 20) { return x; }
