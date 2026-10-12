// P3098: as p3098-capture-copy-run.C, but with the checks outlined.  The
// outlined path initializes a capture-struct member rather than a local, and
// had the same bit-copy.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe -fcontract-checks-outlined" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int copies = 0;
int dtors = 0;

struct S {
  int v;
  S (int x) : v (x) { }
  S (const S &o) : v (o.v) { ++copies; }
  ~S () { ++dtors; }
};

bool ok (int x) { return x == 7; }

void by_param (S p) post [p] (ok (p.v)) { }
void by_init (S p) post [q = p] (ok (q.v)) { }

int main ()
{
  {
    S s (7);
    int c0 = copies, d0 = dtors;
    by_param (s);
    if (copies - c0 != 2)
      __builtin_abort ();
    if (dtors - d0 != 2)
      __builtin_abort ();
  }

  {
    S s (7);
    int c0 = copies, d0 = dtors;
    by_init (s);
    if (copies - c0 != 2)
      __builtin_abort ();
    if (dtors - d0 != 2)
      __builtin_abort ();
  }
}
