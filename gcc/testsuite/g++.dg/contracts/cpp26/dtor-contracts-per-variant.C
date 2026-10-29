// A constructor's or destructor's contracts are evaluated once, whatever ABI
// variants one variant calls another through (deleting -> complete, complete
// -> base) -- except that the virtual delete of S1 also gets the P3097
// interface check, so there S1's contracts are evaluated twice (DECISIONS.md
// K8).
//
// Mirror: clang/test/Contracts/dtor-contracts-deleting-dtor.cpp,
// dtor-contracts-virtual-base.cpp, dtor-contracts-no-ctor-aliases.cpp and
// ctor-contracts-no-ctor-aliases.cpp in the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

static int pres = 0, posts = 0;
bool pre_hit () { ++pres; return true; }
bool post_hit () { ++posts; return true; }

struct S1 {
  virtual ~S1 () pre (pre_hit ()) post (post_hit ()) {}
};

struct V {};
struct S2 : virtual V {
  ~S2 () pre (pre_hit ()) post (post_hit ()) {}
};

struct S3 {
  ~S3 () pre (pre_hit ()) post (post_hit ()) {}
};

struct S4 {
  S4 () pre (pre_hit ()) post (post_hit ()) {}
};

static void check (int n = 1)
{
  if (pres != n || posts != n)
    __builtin_abort ();
  pres = posts = 0;
}

int main ()
{
  S1 *p = new S1;
  delete p;
  check (2);
  { S2 s; }
  check ();
  { S3 s; }
  check ();
  { S4 s; }
  check ();
}
