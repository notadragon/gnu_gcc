// A precondition or postcondition naming `this', implicitly or explicitly,
// on a member function declared in the class and defined outside it is
// checked against the object it is called on; so is the precondition of a
// destructor defined outside the class, and of one defined in it, when the
// destructor runs.  Every check is first satisfied, then each fails once.
// this-param-out-of-class-quick.C checks one of them under quick_enforce.
// (EDG-55 shape: codegen/this_param.)
//
// Mirror: clang/test/Contracts/Runnable/this-param-out-of-class.cpp in the
// llvm_llvm-project fork.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations;
static int lines[8];
void handle_contract_violation (const std::contracts::contract_violation &v)
{
  lines[violations++] = v.location ().line ();
}

struct T
{
  int n;
  int get () const pre (n >= 0) post (r: r == this->n); enum { L1 = __LINE__ };
  void set (int v) pre (this->n != v); enum { L2 = __LINE__ };
  ~T () pre (n != 3); enum { L3 = __LINE__ };
};
int T::get () const { return n; }
void T::set (int v) { n = v; }
T::~T () {}

struct W
{
  int value;
  ~W () pre (value >= 0) {} enum { L4 = __LINE__ };
};

int main ()
{
  { T t{1}; t.set (2); t.get (); W w{0}; }
  if (violations != 0)
    __builtin_abort ();
  { T t{-1}; t.get (); }	// precondition, implicit this
  { T t{2}; t.set (2); }	// precondition, explicit this
  { T t{3}; }			// destructor's precondition
  { W w{-1}; }			// in-class destructor's precondition
  if (violations != 4
      || lines[0] != T::L1 || lines[1] != T::L2 || lines[2] != T::L3
      || lines[3] != W::L4)
    __builtin_abort ();
}
