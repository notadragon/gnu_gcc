// P4405 premises decided by this TU (T1-GCC) at -O0: there is no
// shadow, so folding is final and nothing a callee's assertion implies is
// used, whatever its linkage or inlining; nothing is assumed.
//
// Mirror: clang/test/Contracts/p4405-premise-O0.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O0" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-premise-O0.json" }

int sink (int);

[[gnu::always_inline]] static inline void h_static_always (int x) pre (x > 0) { sink (x); }

int d_static_always (int x)
{
  h_static_always (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

static void h_static_auto (int x) pre (x > 0) { sink (x); }

int d_static_auto (int x)
{
  h_static_auto (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

[[gnu::always_inline]] inline void h_inline_always (int x) pre (x > 0) { sink (x); }

int d_inline_always (int x)
{
  h_inline_always (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

inline void h_inline_auto (int x) pre (x > 0) { sink (x); }

int d_inline_auto (int x)
{
  h_inline_auto (x);
  contract_assert (x > 0);	// { dg-warning "contract assertion cannot be proven" }
  return 0;
}

