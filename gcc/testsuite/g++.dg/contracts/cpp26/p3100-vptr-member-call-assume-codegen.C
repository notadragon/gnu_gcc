// { dg-do compile }
// { dg-options "-std=c++26 -fcontracts-p3100 -fsanitize=vptr -fsanitize-semantic=vptr:assume" }
// P3100 (UBSan routing, assume): a member function call through a glvalue
// whose type is not similar to the object's (ub:expr.ref.member.not.similar)
// would be checked by -fsanitize=vptr ("member call on"); resolving the check
// to "assume" turns it off -- no vptr runtime call is emitted.

struct A { virtual ~A () {} int i = 1; };
struct B { virtual ~B () {} int j = 2; int f (); };

int
call (A &a)
{
  return reinterpret_cast<B &> (a).f ();
}

// { dg-final { scan-assembler-not "__ubsan_handle_dynamic_type_cache_miss" } }
