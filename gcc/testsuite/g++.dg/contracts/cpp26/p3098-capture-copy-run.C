// P3098: A postcondition capture of a class type is copy-initialized, so the
// capture's copy constructor runs.  A bare bit-copy would leave the capture's
// destructor running on an object no constructor ever made -- for an owning
// type, a double free.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
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

// Each call copies twice -- once for the by-value parameter, once for the
// capture -- and destroys both.  Counting only the copy constructor is what
// catches the bit-copy: the destructor count is already correct today.
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
