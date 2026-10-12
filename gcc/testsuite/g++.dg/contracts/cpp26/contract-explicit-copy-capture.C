// In a contract predicate an entity declared outside it is named with const
// type, but a by-copy capture of it is a closure member of the entity's own
// type, so it can be modified in a mutable lambda ([expr.prim.id.unqual]
// example) -- explicitly captured as well as by [=] or an init-capture.  A
// by-reference capture names the entity, and stays const.
//
// Mirror: clang/test/Contracts/contract-explicit-copy-capture.cpp and
// result-binding-lambda-explicit-capture.cpp in the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

void a1 ()
{
  int v = 1;
  contract_assert ([v] () mutable { return ++v; } () > 0);
}

int h1 (int x) pre ([x] () mutable { return ++x; } () > 0) { return x; }

int r1 () post (r: [r] () mutable { return ++r; } () > 0) { return 1; }

template <class T>
T t1 (T x) pre ([x] () mutable { return ++x; } () > 0)
  post (r: [r] () mutable { return ++r; } () > 0)
{
  return x;
}

int u = t1 (1);

void c1 ()
{
  int v = 1;
  contract_assert ([&v] () { return ++v; } () > 0);	// { dg-error "read-only" }
}
