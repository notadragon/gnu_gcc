// this-param-out-of-class-run.C under quick_enforce: the satisfied checks of
// a member function declared in the class and defined outside it, naming
// `this' implicitly and explicitly, and of destructors defined outside and
// inside the class, pass; then the out-of-class destructor's precondition
// fails and traps.  (EDG-55 shape: codegen/this_param.)
//
// Mirror: clang/test/Contracts/Runnable/this-param-out-of-class.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target { c++26 && { i?86-*-linux* x86_64-*-linux* } } } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=quick_enforce" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <csignal>
#include <cstdlib>

static volatile int stage;
static void on_trap (int) { std::_Exit (stage == 1 ? 0 : 1); }

struct T
{
  int n;
  int get () const pre (n >= 0) post (r: r == this->n);
  void set (int v) pre (this->n != v);
  ~T () pre (n != 3);
};
int T::get () const { return n; }
void T::set (int v) { n = v; }
T::~T () {}

struct W
{
  int value;
  ~W () pre (value >= 0) {}
};

int main ()
{
  std::signal (SIGILL, on_trap);
  { T t{1}; t.set (2); t.get (); W w{0}; }
  stage = 1;
  { T t{3}; }	// traps
  return 1;
}
