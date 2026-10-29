// P4405 stage 3: an evaluation the proof shadow duplicates -- by unrolling
// a loop, or in each clone of a constructor, the base-object one used
// through a further derived class too -- is diagnosed once, as is a
// postcondition of a function with several returns.  -fmax-errors=3 makes
// a repeated error fail the test.
//
// Mirror: clang/test/Contracts/p4405-shadow-unroll.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O3 -funroll-loops" }
// { dg-additional-options "-fcontract-evaluation-semantic=prove -fmax-errors=3" }

int sum (const int *a)
{
  int s = 0;
  for (int i = 0; i < 4; ++i)
    {
      contract_assert (a[i] > 0);	// { dg-error "contract assertion cannot be proven" }
      s += a[i];
    }
  return s;
}

void *operator new (decltype (sizeof 0), void *p) noexcept;

struct V { int v; V (int x) : v (x) {} };

// Two constructor bodies (complete and base), one evaluation.
struct D : virtual V
{
  D (int x) : V (x)
  {
    contract_assert (x != 3);		// { dg-error "contract assertion cannot be proven" }
  }
};

D make (int x) { return D (x); }
D *make_base (void *p, int x)
{
  struct E : D { E (int x) : V (x), D (x) {} };
  return new (p) E (x);
}

int returns (int x)
  post (r: r > 0)			// { dg-bogus "cannot be proven|always false" }
  post (r: r > 5)			// { dg-error "contract assertion cannot be proven" }
{
  if (x > 0)
    return x;
  if (x < -5)
    return -x;
  return 1;
}
