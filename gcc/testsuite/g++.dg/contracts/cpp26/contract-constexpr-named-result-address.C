/* A named-result postcondition whose predicate needs the ADDRESS of the
   result must be constant-evaluable.

   Regression test.  The result name was bound by copying the RESULT_DECL's
   VALUE into the constexpr value map:

     if (tree rv = ctx->global->get_value (ctx->call->result_decl))
       ctx->global->put_value (result, rv);

   For a class with a non-trivial destructor that value is a bare
   CONSTRUCTOR, so the result name named a value rather than an object with
   storage.  Reading a member through it worked, but anything needing its
   address -- any member function call, because it passes `this` -- reached

     gcc_checking_assert (TREE_CODE (op) != CONSTRUCTOR);

   in the ADDR_EXPR case of cxx_eval_constant_expression and ICEd:

     internal compiler error: in cxx_eval_constant_expression,
     at cp/constexpr.cc:10602

   Only on a checking build: the assert is a gcc_checking_assert, so a
   release compiler computed the right answer and said nothing.  That is why
   this file pins down all four corners rather than the one shape that was
   reported -- the trigger needs a user-provided destructor AND an
   address-taking use, and the three neighbouring combinations were already
   working and must stay that way.  */

// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontract-evaluation-semantic=enforce" }

/* ---- the three destructor flavours ------------------------------------ */

struct NoDtor
{
  int a;
  constexpr NoDtor (int x) : a (x) {}
  constexpr bool ok () const { return a >= 0; }
};

struct DefaultedDtor
{
  int a;
  constexpr DefaultedDtor (int x) : a (x) {}
  constexpr ~DefaultedDtor () = default;
  constexpr bool ok () const { return a >= 0; }
};

struct UserDtor
{
  int a;
  constexpr UserDtor (int x) : a (x) {}
  constexpr ~UserDtor () {}
  constexpr bool ok () const { return a >= 0; }
};

/* ---- address taken: a member call passes `this` ----------------------- */

constexpr NoDtor
nodtor_call (int s) post (r : r.ok ())
{
  return NoDtor (s);
}

constexpr DefaultedDtor
defaulted_call (int s) post (r : r.ok ())
{
  return DefaultedDtor (s);
}

/* This is the shape that ICEd.  */
constexpr UserDtor
userdtor_call (int s) post (r : r.ok ())
{
  return UserDtor (s);
}

static_assert (nodtor_call (1).a == 1);
static_assert (defaulted_call (2).a == 2);
static_assert (userdtor_call (3).a == 3);

/* ---- address taken: bound to a reference parameter --------------------- */

constexpr bool
userdtor_ok_ref (const UserDtor &u)
{
  return u.a >= 0;
}

constexpr UserDtor
userdtor_by_ref (int s) post (r : userdtor_ok_ref (r))
{
  return UserDtor (s);
}

static_assert (userdtor_by_ref (4).a == 4);

/* ---- no address taken: a member READ was always fine ------------------ */

constexpr UserDtor
userdtor_read (int s) post (r : r.a >= 0)
{
  return UserDtor (s);
}

static_assert (userdtor_read (5).a == 5);

/* ---- the result name used twice, and alongside a re-call -------------- */

constexpr UserDtor
make_userdtor (int s)
{
  return UserDtor (s);
}

constexpr UserDtor
userdtor_twice (int s) post (r : r.ok () && r.ok ())
{
  return UserDtor (s);
}

constexpr UserDtor
userdtor_recall (const int s) post (r : r.ok () && make_userdtor (s).ok ())
{
  return make_userdtor (s);
}

static_assert (userdtor_twice (6).a == 6);
static_assert (userdtor_recall (7).a == 7);

/* ---- the predicate must still be able to FAIL at compile time --------- */

/* A postcondition that is false in a constant expression is a diagnosable
   error, so it cannot be written here; contract-constexpr-*-fail.C covers
   that.  What this file must show is that a fix which makes the address
   available has not also made the predicate unable to observe the real
   value: each static_assert above reads back the value the predicate
   inspected, and the run-time half below checks the same functions when
   they are NOT constant-evaluated.  */

int
main ()
{
  int s = 8;                    /* Not a constant, so these run.  */
  if (nodtor_call (s).a != 8)
    __builtin_abort ();
  if (defaulted_call (s).a != 8)
    __builtin_abort ();
  if (userdtor_call (s).a != 8)
    __builtin_abort ();
  if (userdtor_by_ref (s).a != 8)
    __builtin_abort ();
  if (userdtor_read (s).a != 8)
    __builtin_abort ();
  if (userdtor_twice (s).a != 8)
    __builtin_abort ();
  if (userdtor_recall (s).a != 8)
    __builtin_abort ();
  return 0;
}
