// The P3100 checks of signed overflow and of invalid values, made by the
// middle end, take precedence over the routed sanitizer per leaf
// (DECISIONS.md E125 (a)): with ub:expr.expr.eval.signed.integer.add and
// ub:conv.lval.valid.representation.bool ignored, an overflowing addition
// and an invalid bool load are not reported, while a subtraction and an
// enumeration load, whose leaves keep the default "assume", are checked by
// the sanitizer, routed to the handler.

// { dg-do run }
// { dg-additional-options "-std=c++26 -fcontracts-p3100 -fcontracts-p4298 -fsanitize=signed-integer-overflow,bool,enum" }
// { dg-additional-options "-fsanitize-semantic=signed-integer-overflow:noexcept_observe -fsanitize-semantic=bool:noexcept_observe -fsanitize-semantic=enum:noexcept_observe" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=ub:expr.expr.eval.signed.integer.add:ignore,ub:conv.lval.valid.representation.bool:ignore" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>
#include <climits>
#include <cstdio>
#include <cstring>

static int reports;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++reports;
}

static volatile int vone = 1, vmax = INT_MAX, vmin = INT_MIN;
enum E3 { A0, A1, A2 };
#define NI __attribute__((noinline))
NI int add () { return vmax + vone; }
NI int sub () { return vmin - vone; }
NI bool loadb (const bool *p) { return *p; }
NI E3 loade (const E3 *p) { return *p; }

#define COUNT(name, ...)						\
  do									\
    {									\
      int before = reports;						\
      __VA_ARGS__;							\
      std::printf (name " %d\n", reports - before);			\
    }									\
  while (0)

int
main ()
{
  bool b;
  unsigned char c = 2;
  std::memcpy (&b, &c, 1);
  E3 e;
  int i = 7;
  std::memcpy (&e, &i, sizeof e);
  COUNT ("add", add ());
  COUNT ("sub", sub ());
  COUNT ("bool", loadb (&b));
  COUNT ("enum", loade (&e));
  return 0;
}

// { dg-output "add 0(\n|\r\n|\r)sub 1(\n|\r\n|\r)bool 0(\n|\r\n|\r)enum 1" }
