// OPEN BUG (GCC-1002 in gnu_gcc open-issues/).
//
// D4405 proof shadow: a precondition that reads a member of a by-value
// class-type parameter is never proven at a call, even for a constant
// argument.  The argument is a TARGET_EXPR, which shadow_substitute does
// not substitute for the parameter, so the obligation is reported
// unproven at once.  The same precondition on a const reference parameter
// (byref) or on a scalar parameter (scalar) is proven.  byval's calls
// should be proven too; the XFAILs record that they are not.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405 -O2 -fcontract-evaluation-semantic=prove" }

struct T { int c; };
int byval (T t) pre (t.c == 1);		// { dg-bogus "the predicate of the contract assertion" "GCC-1002" { xfail *-*-* } }
int byref (const T &t) pre (t.c == 1);
int scalar (int c) pre (c == 1);

int f1 () { T a = {1}; return byval (a); }	// { dg-bogus "cannot be proven" "GCC-1002" { xfail *-*-* } }
int f2 () { return byval (T{1}); }		// { dg-bogus "cannot be proven" "GCC-1002" { xfail *-*-* } }
int f3 () { T a = {1}; return byref (a); }	// { dg-bogus "cannot be proven" }
int f4 () { return scalar (1); }		// { dg-bogus "cannot be proven" }
