// gcc-646-pch-twelve-templates.h                                -*-C++-*-
//
// GCC-646, case b: twelve function templates with contracts, declared in a
// precompiled header.  Which of them keep their contract assertions after
// the reload depends on where the trees landed, so one template alone can
// be right by chance (stock trunk 17.0.0 20261110 gets case a's right);
// twelve make a chance pass unlikely, as the regression test
// g++.dg/pch/contract-template-after-pch.C does.  See
// gcc-646-pch-twelve-templates.cpp.

template <class T> constexpr T ct0 (T x) pre (x > 0) { return x; }
template <class T> constexpr T ct1 (T x) pre (x > 0) { return x; }
template <class T> constexpr T ct2 (T x) pre (x > 0) { return x; }
template <class T> constexpr T ct3 (T x) pre (x > 0) { return x; }
template <class T> constexpr T ct4 (T x) pre (x > 0) { return x; }
template <class T> constexpr T ct5 (T x) pre (x > 0) { return x; }
template <class T> constexpr T ct6 (T x) pre (x > 0) { return x; }
template <class T> constexpr T ct7 (T x) pre (x > 0) { return x; }
template <class T> constexpr T ct8 (T x) pre (x > 0) { return x; }
template <class T> constexpr T ct9 (T x) pre (x > 0) { return x; }
template <class T> constexpr T ct10 (T x) pre (x > 0) { return x; }
template <class T> constexpr T ct11 (T x) pre (x > 0) { return x; }
