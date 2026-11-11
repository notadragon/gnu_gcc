// A destructor whose body is empty, whose members need no destruction and
// whose one base has a nontrivial destructor at offset 0 still evaluates
// its own contract assertions, whichever declaration has them, virtual or
// not: it is not made an alias of the base's destructor (Clang did that
// with constructor aliases on, CLANG-869; GCC never aliases a destructor
// to its base's).
//
// Mirror: clang/test/Contracts/dtor-contracts-base-alias.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3097 -fcontract-evaluation-semantic=observe -O2" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

static int pres = 0, posts = 0;
bool pre_hit () { ++pres; return true; }
bool post_hit () { ++posts; return true; }

// Inline and virtual.
struct B { virtual ~B (); };
B::~B () {}
struct D : B { ~D () override pre (pre_hit ()) post (post_hit ()) {} };

// Defined out of line, over a base destructor defined here.
struct NB { ~NB (); };
NB::~NB () {}
struct ND : NB { ~ND () post (post_hit ()); };
ND::~ND () {}

static void check (int npre, int npost)
{
  if (pres != npre || posts != npost)
    __builtin_abort ();
  pres = posts = 0;
}

int main ()
{
  { D d; }
  check (1, 1);
  B *p = new D;
  delete p;
  check (1, 1);
  { ND nd; }
  check (0, 1);
}
