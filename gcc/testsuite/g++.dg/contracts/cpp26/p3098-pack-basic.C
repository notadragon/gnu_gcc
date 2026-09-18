// P3098: Pack capture syntax — basic parsing.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098" }

// Simple pack capture: [args...]
template <typename... Args>
void f1(Args... args)
  post [args...] (true);

// Init-capture pack: [...old = args]
template <typename... Args>
void f2(Args... args)
  post [...old = args] (true);

// Pack capture with predicate using fold expression.
template <typename... Args>
bool all_positive(Args... a) { return (... && (a > 0)); }

template <typename... Args>
bool f3(Args... args)
  post [args...] (r: all_positive(args...));

// Mixed: pack capture + scalar capture.
template <typename... Args>
bool f5(int n, Args... args)
  post [n, args...] (r: n > 0 && all_positive(args...));

// Mixed: scalar init-capture + pack capture.
template <typename... Args>
bool f6(int n, Args... args)
  post [saved_n = n, args...] (r: saved_n > 0 && all_positive(args...));
