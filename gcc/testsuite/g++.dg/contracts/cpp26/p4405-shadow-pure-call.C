// P4405 stage 3: a predicate's calls make a proof conditional only if, once
// the shadow is decided, the function called is not known to be const or
// pure, terminating and not throwing.  So an inline member function not
// declared pure (S::size, std::vector::size), which GCC's analyses find
// pure, keeps the proof sound; a call to an external function (even one
// the proof never evaluates) or a volatile access makes it conditional.
// Both suppress the diagnostic; the dump tells them apart.
//
// Mirror: clang/test/Contracts/p4405-shadow-pure-call.cpp in the
// llvm_llvm-project fork, with its own way of showing the result.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2" }
// { dg-additional-options "-fcontract-evaluation-semantic=diagnose" }
// { dg-additional-options "-fdump-tree-contract_shadow" }

#include <vector>

int sink (int);
extern int log_call ();
volatile int vol;

struct S
{
  int n;
  int size () const { return n; }
};

int member (const S &s)
{
  if (s.size () > 2)
    contract_assert (s.size () > 1);		// line 31
  return sink (s.n);
}

int vector_size (const std::vector<int> &v)
{
  if (v.size () > 2)
    contract_assert (v.size () > 1);		// line 38
  return sink (v.front ());
}

int impure (int x)
{
  if (x > 0)
    contract_assert (x > 0 || log_call ());	// line 45
  return sink (x);
}

int volatile_read (int x)
{
  if (x > 0)
    contract_assert (x > 0 || vol);		// line 52
  return sink (x);
}

// { dg-final { scan-tree-dump {\(line 31\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 38\): sound} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 45\): conditional} "contract_shadow" } }
// { dg-final { scan-tree-dump {\(line 52\): conditional} "contract_shadow" } }
