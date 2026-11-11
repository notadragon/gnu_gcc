// The refinable-body rule for P3097 callers: a function that calls a virtual
// function through its contract wrapper (internal to this translation unit)
// holds the evaluations the wrapper makes, which another translation unit's
// definition of the function may make differently or not at all, so the
// caller is marked (mark_contract_wrapper_caller) -- unless every
// evaluation of the wrapper is settled (contract_wrapper_settled_p).  The
// C-variadic virtual function (vv, whose wrapper passes the arguments on
// with __builtin_va_arg_pack, p3097-variadic-wrapper.C) and the explicitly
// called virtual destructor follow the same rule as an ordinary virtual
// function (vf).  Every function here is noexcept, so its wrapper cannot
// throw: callers of an unmarked h* drop their landing pads, callers of a
// marked one keep them.
//
// Each h* but hn checks its callee's contract assertions at the call (an
// inline trap, six in all); only h, hv and hd are marked.
// h, hv, hd: no label, not settled, marked: g (1) to g (3) stay.
// hs, hvs, hds: quick_enforce labels, settled, not marked: g (5) to g (7)
// go.
// hn: a virtual call without contracts holds no evaluation: g (4) goes.
//
// Mirror: clang/test/Contracts/refinable-body-p3097.cpp (h, hv, hd, hn),
// where vv's and the destructor's contracts are checked at the call rather
// than in a wrapper.  Clang marks every P3097 caller whatever the label, so
// its mirror has no settled half; GCC skips the mark for a wrapper whose
// evaluations are all settled.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontracts-p3400" }
// { dg-additional-options "-fcontract-evaluation-semantic=quick_enforce -O2 -fno-ipa-sra -fdump-tree-optimized" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template<evaluation_semantic... S>
struct allow {
  using assertion_control_object = allow;
  static constexpr evaluation_semantic_set allowed_semantics{S...};
};

inline constexpr allow<quick_enforce> l_quick{};

int g (int);

struct B
{
  virtual int vf (int x) noexcept post (r: r > 0);
  virtual int vv (int x, ...) noexcept pre (x > 0);
  int k;
  virtual ~B () pre (k > 0);
};

struct BS
{
  virtual int vf (int x) noexcept post<l_quick> (r: r > 0);
  virtual int vv (int x, ...) noexcept pre<l_quick> (x > 0);
  int k;
  virtual ~BS () pre<l_quick> (k > 0);
};

struct N
{
  virtual int vn (int x) noexcept;
};

[[gnu::noinline]] inline int h (B *p, int x) { return p->vf (x); }
[[gnu::noinline]] inline int hv (B *p, int x) { return p->vv (x, 1); }
[[gnu::noinline]] inline void hd (B *p) { p->~B (); }
[[gnu::noinline]] inline int hn (N *p, int x) { return p->vn (x); }
[[gnu::noinline]] inline int hs (BS *p, int x) { return p->vf (x); }
[[gnu::noinline]] inline int hvs (BS *p, int x) { return p->vv (x, 1); }
[[gnu::noinline]] inline void hds (BS *p) { p->~BS (); }

int use_h (B *p, int y) { try { return h (p, y); } catch (...) { return g (1); } }
int use_hv (B *p, int y) { try { return hv (p, y); } catch (...) { return g (2); } }
int use_hd (B *p) { try { hd (p); return 0; } catch (...) { return g (3); } }
int use_hn (N *p, int y) { try { return hn (p, y); } catch (...) { return g (4); } }
int use_hs (BS *p, int y) { try { return hs (p, y); } catch (...) { return g (5); } }
int use_hvs (BS *p, int y) { try { return hvs (p, y); } catch (...) { return g (6); } }
int use_hds (BS *p) { try { hds (p); return 0; } catch (...) { return g (7); } }

// { dg-final { scan-tree-dump "g \\(1\\)" "optimized" } }
// { dg-final { scan-tree-dump "g \\(2\\)" "optimized" } }
// { dg-final { scan-tree-dump "g \\(3\\)" "optimized" } }
// { dg-final { scan-tree-dump-not "g \\(4\\)" "optimized" } }
// { dg-final { scan-tree-dump-not "g \\(5\\)" "optimized" } }
// { dg-final { scan-tree-dump-not "g \\(6\\)" "optimized" } }
// { dg-final { scan-tree-dump-not "g \\(7\\)" "optimized" } }
// { dg-final { scan-tree-dump-times "__builtin_trap" 6 "optimized" } }
// { dg-final { scan-tree-dump-times "refinable body" 3 "optimized" } }
