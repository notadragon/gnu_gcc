// P3098: a capture of a class with a user-declared but TRIVIAL destructor.
//
// This crashed the compiler outright -- "internal compiler error: in
// build_cleanup, at cp/decl2.cc:4052", on a release build and with no earlier
// error to mask it.  type_build_dtor_call is true for a user-declared
// destructor even when it is trivial (the call still has to be built so its
// access is checked), but cxx_maybe_build_cleanup then discards the trivial
// call and returns NULL_TREE, which build_cleanup asserts against.
//
// `~T () = default;' is ordinary, so this reached any capture of such a type,
// in both the inline and the outlined lowering.  The implicit-destructor case
// (struct A { int v; }) never tripped it: type_build_dtor_call is false there,
// so no cleanup was attempted at all.  Both are covered below.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct UserTrivial {	    // user-declared destructor, trivial
  int v;
  ~UserTrivial () = default;
};

struct ImplicitTrivial {    // implicit destructor
  int v;
};

bool ok (int x) { return x == 7; }

void user_param (UserTrivial p) post [p] (ok (p.v)) { }
void user_init (UserTrivial p) post [q = p] (ok (q.v)) { }
void implicit_param (ImplicitTrivial p) post [p] (ok (p.v)) { }

int main ()
{
  user_param (UserTrivial{7});
  user_init (UserTrivial{7});
  implicit_param (ImplicitTrivial{7});
}
