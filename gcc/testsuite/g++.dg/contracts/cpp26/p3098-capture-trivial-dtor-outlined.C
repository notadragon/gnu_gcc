// P3098: as p3098-capture-trivial-dtor.C, but with the checks outlined.  The
// outlined lowering destroys capture-struct members through a second
// build_cleanup call site and crashed identically.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe -fcontract-checks-outlined" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

struct UserTrivial {
  int v;
  ~UserTrivial () = default;
};

bool ok (int x) { return x == 7; }

void user_param (UserTrivial p) post [p] (ok (p.v)) { }
void user_init (UserTrivial p) post [q = p] (ok (q.v)) { }

int main ()
{
  user_param (UserTrivial{7});
  user_init (UserTrivial{7});
}
