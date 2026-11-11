// A function's name is in scope from the end of its declarator
// ([basic.scope.pdecl]), so it can be named in its own contracts.
//
// Mirror: clang/test/Contracts/self-name-in-own-contract.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

int rec (int x) pre (x == 0 || rec (x - 1) >= 0);
int rec2 (int x) pre (x == 0 || rec2 (x - 1) >= 0) { return x; }
constexpr int rec3 (const int x) post (r : x == 0 || rec3 (x - 1) == r - 1) { return x; }
static_assert (rec3 (3) == 3);

template <class T> T trec (T x) pre (x == 0 || trec (x - 1) >= 0) { return x; }
int use_trec = trec (2);

int a, grp (int x) pre (x == 0 || grp (x - 1) >= 0);

struct S {
  int m (int x) const pre (x == 0 || m (x - 1) >= 0);
  static int sm (int x) pre (x == 0 || sm (x - 1) >= 0);
};
int S::m (int x) const pre (x == 0 || m (x - 1) >= 0) { return x; }
int S::sm (int x) pre (x == 0 || sm (x - 1) >= 0) { return x; }

void blk ()
{
  int local (int x) pre (x == 0 || local (x - 1) >= 0);
}
