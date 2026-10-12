// An init-capture pack `post [...c = t]` can be expanded in the predicate --
// in a call, a fold and sizeof... -- and each element has its own capture.
//
// Mirror: clang/test/Contracts/Runnable/init-capture-pack-in-predicate.cpp in
// the llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

template <class... A> int sum (A... a) { return (0 + ... + a); }

template <class... T>
int f (T... t) post [...c = t] (r : sum (c...) == 3) { return 0; }

template <class... T>
int g (T... t) post [...c = t] (r : ((c > 0) && ...)) { return 0; }

int main () {
  f (1, 2);
  g (1, 2);
}
