// A variable, reference or structured binding named in a contract_assert
// through a by-reference capture of the enclosing lambda is "declared
// outside of C" and so const ([expr.prim.id.unqual]).  The by-copy sibling
// is contract-copy-capture-constify.C.
//
// Mirror: clang/test/Contracts/contract-ref-capture-constify.cpp in
// the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

struct S { int a, b; };

void g (S s, int &r)
{
  auto [x, y] = s;
  auto &[p, q] = s;
  contract_assert (++x);			// { dg-error "read-only" }
  contract_assert (++p);			// { dg-error "read-only" }
  contract_assert (++r);			// { dg-error "read-only" }
  [&x] { contract_assert (++x); } ();		// { dg-error "read-only" }
  [&r] { contract_assert (++r); } ();		// { dg-error "read-only" }
  [&k = r] { contract_assert (++k); } ();	// { dg-error "read-only" }
  int v = 0;
  [&v] { contract_assert (++v); } ();		// { dg-error "read-only" }
}

void a0 (int x)
{
  auto l = [&] { contract_assert (++x > 0); (void) x; }; // { dg-error "read-only" }
  l ();
}

void a1 (int x)
{
  auto l = [&] (auto u) { contract_assert (++x > u); (void) x; }; // { dg-error "read-only" }
  l (0);
}

template <class T>
void b (T x)
{
  auto l = [&] { contract_assert (++x > 0); (void) x; }; // { dg-error "read-only" }
  l ();
}

template void b<int> (int);
