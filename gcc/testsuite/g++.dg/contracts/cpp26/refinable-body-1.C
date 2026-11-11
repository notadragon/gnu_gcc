// The evaluation semantic of a contract assertion can differ between
// translation units, so a function another unit may also define (inline
// function, template instantiation, member function defined in its class,
// lambda) has a refinable body when it holds a contract assertion: its
// callers must not take "cannot throw" from it, because another unit's
// definition may call a throwing violation handler.  Here every assertion
// is ignored, so none of the bodies can throw, but the calls of the
// functions that may be defined elsewhere keep their landing pads.  The
// static and the external non-inline function have no other definition,
// and the inline fn holds no assertion, so their callers lose theirs.
//
// Mirror: clang/test/Contracts/refinable-body-nounwind.cpp
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=ignore -O2 -fno-ipa-sra -fdump-tree-optimized -fdump-ipa-inline" }

int g (int);

[[gnu::noinline]] inline int fi (int x) { contract_assert (x > 0); return x; }

template <typename T>
[[gnu::noinline]] T ft (T x) pre (x > 0) { return x; }

struct S
{
  int v;
  [[gnu::noinline]] S (int x) pre (x > 0) : v (x) { }
  [[gnu::noinline]] ~S () post (this->v != 0) { }
  [[gnu::noinline]] int get () const { contract_assert (v > 0); return v; }
};

inline auto lam = [] [[gnu::noinline]] (int x) { contract_assert (x > 0); return x; };

[[gnu::noinline]] static int fs (int x) { contract_assert (x > 0); return x; }

[[gnu::noinline]] int fe (int x) { contract_assert (x > 0); return x; }

[[gnu::noinline]] inline int fn (int x) { return x; }

int use_i (int y) { try { return fi (y); } catch (...) { return g (1); } }
int use_t (int y) { try { return ft (y); } catch (...) { return g (2); } }
int use_c (int y) { try { S s (y); return 0; } catch (...) { return g (3); } }
int use_m (const S &s) { try { return s.get (); } catch (...) { return g (4); } }
int use_l (int y) { try { return lam (y); } catch (...) { return g (5); } }
int use_s (int y) { try { return fs (y); } catch (...) { return g (6); } }
int use_e (int y) { try { return fe (y); } catch (...) { return g (7); } }
int use_n (int y) { try { return fn (y); } catch (...) { return g (8); } }

// { dg-final { scan-tree-dump "g \\(1\\)" "optimized" } }
// { dg-final { scan-tree-dump "g \\(2\\)" "optimized" } }
// { dg-final { scan-tree-dump "g \\(3\\)" "optimized" } }
// { dg-final { scan-tree-dump "g \\(4\\)" "optimized" } }
// { dg-final { scan-tree-dump "g \\(5\\)" "optimized" } }
// { dg-final { scan-tree-dump-not "g \\(6\\)" "optimized" } }
// { dg-final { scan-tree-dump-not "g \\(7\\)" "optimized" } }
// { dg-final { scan-tree-dump-not "g \\(8\\)" "optimized" } }
// { dg-final { scan-ipa-dump "Availability: overwritable \\\(refinable body\\\)" "inline" } }
