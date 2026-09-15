// A postcondition on a void function (with no result-name) is checked when
// the function returns -- both when falling off the end of the body and on an
// explicit "return;".  The side-effecting predicate lets us observe that it
// ran the expected number of times.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

static int post_count = 0;

bool
count_post ()
{
  ++post_count;
  return true;
}

void
implicit_return () post (count_post ())
{
}

void
explicit_return () post (count_post ())
{
  return;
}

int
main ()
{
  post_count = 0;
  implicit_return ();
  if (post_count != 1)
    __builtin_abort ();
  explicit_return ();
  if (post_count != 2)
    __builtin_abort ();
}
