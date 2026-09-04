// [expr.prim.id.unqual]/3+d: inside the predicate of a contract assertion C,
// an id-expression naming "a variable declared outside of C of object type T"
// has type const T.  There is NO storage-duration restriction -- P2900R9
// removed one ("Made implicit const in contract predicates apply to all
// variables, rather than just those with automatic storage duration"), and
// the paragraph's own example opens with a namespace-scope `int n` and
// `pre(++n) // error: attempting to modify const lvalue`.
//
// Only automatic-storage variables were covered by the testsuite before.
// Clang constifies only those, so its mirror of this file is XFAILed.
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

struct X
{
  bool m ();
};

// The paragraph's example.
int n = 0;

struct Y
{
  int z = 0;

  void f (int i, int *p, int &r, X x, X *px)
      pre (++n)      // { dg-error "increment of read-only location" }
      pre (++i)      // { dg-error "increment of read-only location" }
      pre (++(*p))   // OK: the pointee is not const
      pre (++r)      // { dg-error "increment of read-only location" }
      pre (x.m ())   // { dg-error "discards qualifiers" }
      pre (px->m ()) // OK: only the pointer is const
  {
  }
};

// A namespace-scope variable on its own.
int g_n = 0;
void f_namespace () pre (++g_n); // { dg-error "increment of read-only location" }

// A namespace-scope POINTER: the pointer is const, the pointee is not.
int g_i = 0;
int *g_p = &g_i;
void f_pointer_itself () pre (++g_p); // { dg-error "increment of read-only location" }
void f_pointee () pre (++*g_p);       // OK

// Assignment through the pointer variable, the shape in which a
// missing-constification bug hides behind a different diagnostic: assigning
// `const int *` to `int *` is ill-formed for its own reason, so a compiler
// that fails to constify `g_y` still rejects this -- for the wrong reason.
// Incrementing above is the shape that isolates it.
int *g_y = nullptr;
void
f_assign ()
{
  int a = 0;
  contract_assert ((g_y = &a) != nullptr); // { dg-error "assignment of read-only location|discards qualifiers" }
}

// A function-local static.
void
f_local_static ()
{
  static int s = 0;
  contract_assert (++s); // { dg-error "increment of read-only location" }
}

// A thread_local.
thread_local int t_n = 0;
void f_thread_local () pre (++t_n); // { dg-error "increment of read-only location" }

// A static data member -- a variable, so the rule reaches it.
struct HasStatic
{
  static int s;
};
int HasStatic::s = 0;
void f_static_member () pre (++HasStatic::s); // { dg-error "increment of read-only location" }

// CONTROL: a local of automatic storage duration, the case every
// implementation already handles.
void
f_automatic ()
{
  int a = 0;
  contract_assert (++a); // { dg-error "increment of read-only location" }
}
