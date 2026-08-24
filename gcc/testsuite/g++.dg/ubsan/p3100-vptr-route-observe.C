// P3100 Task 4.1 (UBSan runtime routing): under -fcontracts-p3100 a UBSan
// runtime check's report is routed to the contract-violation handler.  vptr is
// the first routed UBSan check.  vptr is always recoverable, so with
// -fcontracts-p4298 the default -fsanitize=vptr resolves to noexcept_observe:
// the handler runs (reporting an implicit contract assertion) and the program
// CONTINUES.  Assert kind() == implicit (7) and semantic() == noexcept_observe
// (6), NOT the plain throwing observe (2).

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=vptr" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }
// The routing descriptor is a preserved weak global emitted in the front-end
// compile, so routing works identically under -flto -- no LTO variants skipped.

#include <contracts>
#include <cstdio>

// Non-throwing handler.  A throwing handler here would std::terminate() from
// inside libubsan's noexcept ScopedReport destructor (Task 4.1), which is
// exactly why the routed semantic is noexcept_observe.
void handle_contract_violation (const std::contracts::contract_violation &v)
{
  std::printf ("handler kind=%d semantic=%d\n", (int) v.kind (),
	       (int) v.semantic ());
  std::fflush (stdout);
}

struct S { S () : a (0) {} virtual int v () { return 0; } int a; };
struct T : S { T () : b (0) {} int b; };

int __attribute__((noinline))
access_b (T *p)
{
  return p->b;  // vptr: member access; *p is really an S, not a T.
}

int main ()
{
  S s;
  T *p = reinterpret_cast<T *> (&s);
  volatile int sink = access_b (p);
  (void) sink;
  // noexcept_observe = report + continue: we reach here after the violation.
  std::printf ("survived\n");
  std::fflush (stdout);
  return 0;
}

// RF5: the sanitizer emits NOTHING on the routed path (this handler does not
// call report()), so the handler line is the FIRST output -- anchor at ^ so any
// leaked pre-handler UBSan text ("runtime error: ...") would fail the match.
// { dg-output "^handler kind=7 semantic=6" }
// noexcept_observe continues: "survived" is printed after the detected access.
// { dg-output ".*survived" }
