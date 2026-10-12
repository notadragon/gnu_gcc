// An array declared outside a contract predicate is const in it
// ([expr.prim.id.unqual]), and so is the array a lambda in the predicate
// captures by reference: of constant size or of runtime bound (a GNU
// extension), captured implicitly ([&]) or explicitly ([&a]), it can be
// neither modified nor passed to a pointer to non-const.  A VLA is captured
// as a pointer and a bound, and an implicit capture of one used to capture
// a pointer to non-const elements.  The same holds in a template, and for
// an array the predicate's lambda reaches through a capture of an enclosing
// lambda (contract-enclosing-lambda-capture-constify.C covers that for
// every kind of entity).
//
// Mirror: clang/test/Contracts/contract-array-ref-capture-constify.cpp in
// the llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -Wno-vla" }

int take (int *);
int look (const int *);

void
fixed ()
{
  int a[3] = {};
  contract_assert ([&] { return ++a[0] == 0; } ());	// { dg-error "read-only" }
  contract_assert ([&a] { return ++a[0] == 0; } ());	// { dg-error "read-only" }
  contract_assert ([&] { return take (a); } ());		// { dg-error "invalid conversion" }
  contract_assert ([&a] { return take (a); } ());	// { dg-error "invalid conversion" }
  contract_assert ([&] { return look (a); } ());
  contract_assert ([&a] { return look (a) + a[1]; } ());
}

void
vla (int n)
{
  int a[n];
  a[0] = 0;
  contract_assert ([&] { return ++a[0] == 0; } ());	// { dg-error "read-only" }
  contract_assert ([&a] { return ++a[0] == 0; } ());	// { dg-error "read-only" }
  contract_assert ([&] { return take (a); } ());		// { dg-error "invalid conversion" }
  contract_assert ([&a] { return take (a); } ());	// { dg-error "invalid conversion" }
  contract_assert ([&] { return take (&a[0]); } ());	// { dg-error "invalid conversion" }
  contract_assert ([&] { int *p = a; return p != 0; } ()); // { dg-error "invalid conversion" }
  contract_assert ([&] { a[1] = 1; return true; } ());	// { dg-error "read-only" }
  contract_assert ([&] { return look (a) + a[0]; } ());
  contract_assert ([&a] { return look (a) + a[0]; } ());
  contract_assert ([&] { return [&] { return ++a[0]; } (); } ()); // { dg-error "read-only" }
}

// A lambda in the predicate whose by-copy capture is the array's copy, or
// that declares an array itself, may modify it.
void
inside ()
{
  int a[3] = {};
  contract_assert ([=] () mutable { return ++a[0] == 1; } ());
  contract_assert ([] { int b[3] = {}; return [&] { return ++b[0]; } (); } ());
}

void
inside_vla (int n)
{
  contract_assert ([n] {
    int b[n];
    return [&] { b[0] = 1; return take (b); } ();
  } ());
}

template <class T>
void
tfixed ()
{
  T a[3] = {};
  contract_assert ([&] { return ++a[0] == 0; } ());	// { dg-error "read-only" }
  contract_assert ([&] { return take (a); } ());		// { dg-error "invalid conversion" }
}
template void tfixed<int> ();

template <class T>
void
tvla (int n)
{
  T a[n];
  contract_assert ([&] { return ++a[0] == 0; } ());	// { dg-error "read-only" }
  contract_assert ([&] { return take (a); } ());		// { dg-error "invalid conversion" }
}
template void tvla<int> (int);

template <class T>
void
tvla_nondep (int n)
{
  int a[n];
  contract_assert ([&] { return ++a[0] == 0; } ());	// { dg-error "read-only" }
  contract_assert ([&a] { return take (a); } ());	// { dg-error "invalid conversion" }
}
template void tvla_nondep<int> (int);
