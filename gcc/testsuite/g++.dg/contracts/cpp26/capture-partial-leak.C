// When a postcondition capture initializer throws, the captures of the same
// postcondition that were already initialized are destroyed before the
// post_capture violation is reported ([basic.contract.eval]).  Under observe
// execution continues, and nothing is left live.  The outlined form is
// capture-partial-leak-outlined.C.
//
// Mirror: clang/test/Contracts/capture-partial-leak.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

static int live = 0;
struct Good {
  Good () { ++live; }
  Good (const Good&) { ++live; }
  ~Good () { --live; }
};
struct Bad {
  Bad () { throw 42; }
};

int f (int i)
  post [g = Good (), b = Bad ()] (true)
{
  return i;
}

int main ()
{
  f (1);
  if (live != 0)
    __builtin_abort ();
}
