// A result-name introducer is only meaningful on a postcondition.  It is
// nevertheless parsed on every contract kind so that the real problem is
// reported, instead of the identifier falling through to the predicate and
// producing an unrelated "found ':' in nested-name-specifier" cascade.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

int f(int r) pre(r: r > 0) { return r; } // { dg-error "result name .r. not allowed outside of post condition specifier" }

void g(int r) {
  contract_assert(r: r > 0); // { dg-error "result name .r. not allowed outside of post condition specifier" }
}

// The valid case still works.
int ok(int x) post(r: r > 0) { return x; }
