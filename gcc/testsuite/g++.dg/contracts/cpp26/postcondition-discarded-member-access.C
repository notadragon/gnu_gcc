// A by-value parameter named as the object expression of a discarded member
// access in a postcondition is not odr-used, so it need not be const
// ([basic.def.odr]: the potential results of a member access naming a
// non-static data member include those of its object expression;
// [dcl.contract.func]) (GCC-650).  The discarded parameter itself is
// pr126897.C (GCC-19).  A volatile member access, to which the
// lvalue-to-rvalue conversion is applied, still odr-uses it.
//
// Nor does such a postcondition reach a redeclaration that declares the
// parameter without const; a redeclaration of one whose postcondition does
// odr-use it must declare it const (GCC-662).
//
// Mirror: clang/test/Contracts/postcondition-discarded-member-access.cpp in
// the llvm_llvm-project fork (CLANG-637, CLANG-646).  Found by the EDG fork's
// sema/postcondition_discarded_params test.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce -Wno-unused-value" }

struct S { int m; bool ok () const; };
struct T { S in; };

void mem (S s) post ((s.m, true)) {}	// { dg-bogus "postcondition must be const" }
void mem_void (S s) post (((void) s.m, true)) {}	// { dg-bogus "postcondition must be const" }
void mem_nested (T t) post ((t.in.m, true)) {}	// { dg-bogus "postcondition must be const" }
void whole (S s) post ((s, true)) {}	// OK, as pr126897.C
void call (S s) post ((s.ok (), true)) {}	// { dg-error "postcondition must be const" }

struct V { volatile int m; };
void vol (V v) post ((v.m, true)) {}	// { dg-error "postcondition must be const" }

// Redeclarations.
void re_whole (S s) post ((s, true));
void re_whole (S s);			// { dg-bogus "postcondition must be const" }
void re_mem (S s) post ((s.m, true));
void re_mem (S s);			// { dg-bogus "postcondition must be const" }
void re_whole_c (const S s) post ((s, true));
void re_whole_c (S s) {}		// { dg-bogus "postcondition must be const" }
void re_mem_c (const S s) post ((s.m, true));
void re_mem_c (S s) {}			// { dg-bogus "postcondition must be const" }
void re_call (const S s) post ((s.ok (), true));
void re_call (S s);			// { dg-error "postcondition must be const" }
