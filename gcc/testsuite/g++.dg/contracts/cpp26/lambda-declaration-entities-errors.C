// A lambda in a precondition or postcondition: what it captures by reference
// is const, so only the copies of a mutable lambda can be modified; a lambda
// capturing a value parameter in a postcondition, implicitly or explicitly,
// odr-uses it, so it must be const (sizeof does not); and an implicit capture
// needs a capture-default, including from a nested lambda.  (EDG-1/2/39
// shapes.)  The [this] shape, a lambda modifying *this, is the open-bug
// watch test for a separate defect.
//
// Mirror: clang/test/Contracts/lambda-declaration-entities-errors.cpp in the
// llvm_llvm-project fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=quick_enforce" }

void a (int x) pre ([&] { return ++x > 0; } ());                 // { dg-error "read-only" }
void b (int x) pre ([&x] { return ++x > 0; } ());                // { dg-error "read-only" }
void c (int x) pre ([x] () mutable { return ++x > 0; } ());      // OK, a copy
void d (int x) pre ([x] { return ++x > 0; } ());                 // { dg-error "read-only" }
int e () post (r: [&r] { return ++r > 0; } ());                  // { dg-error "read-only" }
int e2 () post (r: [r] () mutable { return ++r > 0; } ());       // OK, a copy
int p (int x) post ([&] { return x > 0; } ());                   // { dg-error "value parameter used in a postcondition must be const" }
int p2 (int x) post ([x] { return x > 0; } ());                  // { dg-error "value parameter used in a postcondition must be const" }
int p3 (int x) post ([&] { return sizeof (x) > 0; } ());         // OK, unevaluated
int p4 (const int x) post ([&] { return x > 0; } ());            // OK
void n (int x) pre ([] { return x > 0; } ());                    // { dg-error "not captured" }
void n2 (int x) pre ([=] { return [] { return x > 0; } (); } ()); // { dg-error "not captured" }
