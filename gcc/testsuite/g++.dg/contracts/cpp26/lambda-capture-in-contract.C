// [expr.prim.lambda.capture]/3.3 allows a capture-default or simple-capture in
// a lambda-introducer when the lambda appears within a contract assertion and
// its innermost enclosing scope is the corresponding contract-assertion scope.
// So a lambda written in a contract predicate may capture the enclosing
// function's parameters.
//
// Every case is checked BY THE VALUE THE PREDICATE SAW, not by whether it
// compiles: a lambda that captured the wrong entity, or read an uninitialised
// closure field, would still compile.  A contract predicate cannot assign to a
// variable (the predicate constifies everything it names), so the value is
// recorded through a called function.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts" }

int g_seen = -1;

bool
record (int v)
{
  g_seen = v;
  return true;
}

/* Free functions: each of the four legal capture forms.  */
void copy_cap (int x) pre ([x] { return record (x); } ()) { }
void ref_cap (int x) pre ([&x] { return record (x); } ()) { }
void default_ref (int x) pre ([&] { return record (x); } ()) { }
void default_copy (int x) pre ([=] { return record (x); } ()) { }

/* A postcondition, which is where a captured parameter is most likely to be
   confused with the result object.  The parameter must be const: capturing it
   odr-uses it, and [dcl.contract.func] requires a non-reference parameter
   odr-used by a postcondition predicate to have const type.

   Deliberately written WITHOUT a result-name-introducer.  A lambda in a free
   function's postcondition that does have one -- post (r : []{...}()) -- does
   not parse, which is an unrelated and pre-existing upstream defect (it needs
   no capture, and reproduces on stock g++ 16.2.0 and g++-trunk); see
   gcc-11 in ../../../../../notadragon_wg21 p3850impl/gcc-bugs.  The member
   function equivalent below parses fine, so this file still covers the
   result-name interaction on that side.  */
int post_cap (const int x) post ([x] { return record (x); } ())
{ return x + 1; }

/* Member functions: the parameters are reparented at a different point than
   for a free function, and 'this' is remapped separately.  */
struct S
{
  int m = 7;

  void mem (int x) pre ([x] { return record (x); } ()) { }
  void mem_default (int x) pre ([&] { return record (x); } ()) { }

  /* The result-name-introducer form, which parses on the member path.  */
  int mem_post (const int x) post (r : [x] { return record (x); } ())
  { return x + 1; }

  /* A capture of a parameter alongside 'this', which reaches the member
     through the remapped dummy rather than through a capture.  */
  void mem_this (int x) pre ([this, x] { return record (x + m); } ()) { }
};

/* A lambda's own precondition capturing that lambda's parameter.  */
void
lambda_own_pre ()
{
  auto l = [] (int x) pre ([x] { return record (x); } ()) { };
  l (66);
}

int
main ()
{
  copy_cap (11);
  if (g_seen != 11)
    __builtin_abort ();

  ref_cap (22);
  if (g_seen != 22)
    __builtin_abort ();

  default_ref (33);
  if (g_seen != 33)
    __builtin_abort ();

  default_copy (44);
  if (g_seen != 44)
    __builtin_abort ();

  post_cap (55);
  if (g_seen != 55)
    __builtin_abort ();

  S s;

  s.mem (77);
  if (g_seen != 77)
    __builtin_abort ();

  s.mem_default (88);
  if (g_seen != 88)
    __builtin_abort ();

  s.mem_this (99);
  if (g_seen != 99 + 7)
    __builtin_abort ();

  s.mem_post (111);
  if (g_seen != 111)
    __builtin_abort ();

  lambda_own_pre ();
  if (g_seen != 66)
    __builtin_abort ();

  return 0;
}
