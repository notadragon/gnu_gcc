/* A _Pre or _Post may only appear on a declarator that declares a function.

   GCC-48.  c_parser_direct_declarator_inner accepts a contract specifier
   after any parameter list, because that is the only point at which the
   tokens can be recognised -- but a parameter list appears in plenty of
   declarators that declare something other than a function.  In those the
   specifier applied to nothing, and because the saved tokens are only ever
   replayed into a function body it was not merely dropped but never parsed:
   before this, every case in the first group below compiled silently, and
   `_Pre (nosuch > 0)' with no `nosuch' in scope produced no diagnostic at
   all.

   D4299 states that the semantics of C contract assertions are identical to
   C++26, where [dcl.contract.func]/1 makes the specifier part of a function
   declarator; the C++ front end refuses all of these, and the wording here
   is deliberately the same so the two read alike.

   A contract on a *prototype* is not in this family.  D4299 gives it no
   effect and it is discarded in silence, which contracts-prototype-*.c
   covers.  */

/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299" } */

/* ---- silently accepted before GCC-48 ---------------------------------- */

typedef int F (int) _Pre (nosuch > 0);		/* { dg-error "cannot appear on a typedef" } */

int (*fnptr) (int) _Pre (nosuch > 0);		/* { dg-error "cannot appear on a declaration of non-function type" } */

struct S {
  int (*member) (int) _Pre (nosuch > 0);	/* { dg-error "cannot appear on a declaration of non-function type" } */
};

unsigned by_sizeof = sizeof (int (*) (int) _Pre (nosuch > 0)); /* { dg-error "cannot appear on a type-name" } */

void
by_cast (void)
{
  (void) (int (*) (int) _Pre (nosuch > 0)) 0;	/* { dg-error "cannot appear on a type-name" } */
}

/* ---- warned about, but accepted, before GCC-48 ------------------------ */

void takes_fn (int bar () _Pre (nosuch > 0));	/* { dg-error "cannot appear on a parameter" } */

void defines_fn (int bar () _Pre (nosuch > 0)) { } /* { dg-error "cannot appear on a parameter" } */

/* ---- already rejected by the grammar, and must stay that way ---------- */

int obj _Pre (1);			/* { dg-error "expected" } */

int arr[3] _Pre (1);			/* { dg-error "expected" } */

struct B {
  int bf : 3 _Pre (1);			/* { dg-error "expected" } */
};

/* ---- valid, and must keep compiling ----------------------------------- */

int good_proto (int a) _Pre (a > 0);

int
good_defn (int a) _Pre (a > 0)
{
  return a;
}

/* A function-pointer parameter without a contract of its own.  */
int
good_fnptr_parm (void (*cb) (int), int a) _Pre (a > 0)
{
  (void) cb;
  return a;
}

/* Old-style definition: the K&R parameter loop nulls the pending-contract
   globals around its recursive declaration parses, so a check on them must
   not fire against that deliberately-empty vector.  The -Wold-style-definition
   this draws is the suite's, not this test's.  */
int
good_kandr (a) int a;	/* { dg-warning "old-style function definition" } */
{
  return a;
}
