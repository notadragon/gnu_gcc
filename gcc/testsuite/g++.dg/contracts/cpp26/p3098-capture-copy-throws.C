// P3098: a throwing copy constructor on a capture is a capture-initialization
// exception, and so a post_capture violation.  This path is only reachable
// once the capture is genuinely copy-initialized; a bit-copy cannot throw.
// The parameter is taken by reference so the only copy is the capture's.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

int predicate_count = 0;
bool count_predicate () { ++predicate_count; return true; }

struct Throwing {
  int v;
  Throwing (int x) : v (x) { }
  Throwing (const Throwing &) { throw 42; }
  ~Throwing () { }
};

int f (const Throwing &p)
  post [q = p] (count_predicate ())
{
  return p.v;
}

int main ()
{
  Throwing t (10);
  int result = f (t);

  // The handler returned, so execution continues and the predicate is
  // skipped -- the capture it would read was never built.
  if (result != 10)
    __builtin_abort ();
  if (predicate_count != 0)
    __builtin_abort ();
}
// { dg-output "contract violation in function int f.const Throwing.. at .*(\n|\r\n|\r)" }
// { dg-output ".assertion_kind: post_capture, semantic: observe, mode: evaluation_exception.*terminating: no.*(\n|\r\n|\r)" }
