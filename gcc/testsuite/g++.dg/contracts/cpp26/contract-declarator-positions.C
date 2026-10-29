// A contract specifier is part of a function DECLARATOR, so writing one on a
// declarator that merely has a function TYPE must be rejected.  The two are
// easy to conflate: `FAlias g_alias` declares something whose type is
// `int(int)`, and a parser that asks "is this type a function type?" rather
// than "did this declarator write a parameter list?" accepts it.
//
// All three rows below are rejected with a contract-specific diagnostic.
// They used to come out as a generic "expected initializer before 'pre'",
// which said nothing about contracts; the wording improved when the seq
// started being parsed at its grammar position, where a declarator that
// writes no parameter list has no function declarator to carry one and the
// parser can say so.  This test is where that improvement is pinned.
//
// The MIRROR is clang/test/Contracts/contract-declarator-positions.cpp, which
// covers the same rows plus a member declared through the alias and a
// function-typed parameter.  Neither is here: the member shape produces a
// four-message cascade in GCC's parser that is not worth pinning line by
// line, and the function-typed parameter now has a contract-specific
// diagnostic of its own, pinned in contract-on-non-function-declarator.C
// along with the four other declarators a contract may not occupy.
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

using FAlias = int(int);
FAlias g_alias pre(true);           // { dg-error "cannot appear on a declaration of non-function type" }

typedef int FTypedef(int);
FTypedef g_typedef pre(true);       // { dg-error "cannot appear on a declaration of non-function type" }

// The post spelling, so a fix that only guards `pre` is still caught.
FAlias g_alias_post post(r : true); // { dg-error "cannot appear on a declaration of non-function type" }

// The well-formed counterparts, as controls: these declare functions by
// writing a parameter list, so the contract has a declarator to attach to.
int ok_free(int x) pre(x > 0);

struct T {
  int ok_member(int x) pre(x > 0);
};
