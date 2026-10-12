// P3400: a group name held in a character array is read whole, whatever
// form its constant value takes: a long braced list of byte values (which
// the parser can turn into a single RAW_DATA_CST), and a trailing part the
// initializer omits (zero).  The reader used to take each element of a
// CONSTRUCTOR value as the next byte and write anything but an INTEGER_CST
// as a zero byte, truncating the name (GCC-197); these forms reach it as a
// STRING_CST or with dense elements today, so this is coverage rather than
// a regression test.
// { dg-do run { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce -fcontract-group-evaluation-semantic=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa:observe -fcontract-group-evaluation-semantic=bcd:observe" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

static int violations = 0;
void handle_contract_violation (const std::contracts::contract_violation &)
{ ++violations; }

struct raw_t {
  using assertion_control_object = raw_t;
  static constexpr char group_names[1][80] = { { 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97, 97 } };
};
constexpr raw_t raw{};

struct short_t {
  using assertion_control_object = short_t;
  static constexpr char group_names[1][8] = { { 'b', 'c', 'd' } };
};
constexpr short_t short_name{};

void f (int x) pre<raw> (x > 0) {}
void g (int x) pre<short_name> (x > 0) {}

int main ()
{
  f (-1);
  g (-1);
  return violations == 2 ? 0 : 1;
}
