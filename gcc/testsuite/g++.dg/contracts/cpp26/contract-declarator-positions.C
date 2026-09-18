// A contract specifier is part of a function DECLARATOR, so writing one on a
// declarator that merely has a function TYPE must be rejected.  The two are
// easy to conflate: `FAlias g_alias` declares something whose type is
// `int(int)`, and a parser that asks "is this type a function type?" rather
// than "did this declarator write a parameter list?" accepts it.
//
// GCC rejects all three rows below, but only as a generic parse failure --
// there is no contract-specific diagnostic, so the user is told `pre` is not
// a valid initializer rather than that a contract cannot go here.  That is
// worth pinning as-is: the verdict is right, and if the wording ever improves
// this test is where that shows up.
//
// The MIRROR is clang/test/Contracts/contract-declarator-positions.cpp, which
// covers the same rows plus a member declared through the alias and a
// function-typed parameter.  Neither is here: the member shape produces a
// four-message cascade in GCC's parser that is not worth pinning line by
// line, and the function-typed parameter is ACCEPTED by GCC -- an open bug,
// xfailed in open-bug-contract-on-function-typedef.C.
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

using FAlias = int(int);
FAlias g_alias pre(true);           // { dg-error "expected initializer before 'pre'" }

typedef int FTypedef(int);
FTypedef g_typedef pre(true);       // { dg-error "expected initializer before 'pre'" }

// The post spelling, so a fix that only guards `pre` is still caught.
FAlias g_alias_post post(r : true); // { dg-error "expected initializer before 'post'" }

// The well-formed counterparts, as controls: these declare functions by
// writing a parameter list, so the contract has a declarator to attach to.
int ok_free(int x) pre(x > 0);

struct T {
  int ok_member(int x) pre(x > 0);
};
