// gcc-646-pch-template-contracts.h: the precompiled header.
template <class T> constexpr T ct (T x) pre (x > 0) post (r: r < 9) { return x; }
