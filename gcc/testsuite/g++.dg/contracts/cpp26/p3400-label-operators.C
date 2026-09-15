// P3400: Label type with overloaded < and > operators; parenthesized expression.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3400" }

struct weird_label_t {
  using assertion_control_object = weird_label_t;
  constexpr bool operator<(const weird_label_t&) const { return false; }
  constexpr bool operator>(const weird_label_t&) const { return true; }
};
constexpr weird_label_t weird_label;

// Unparenthesized use of a label with overloaded operators works normally.
void f(int x)
  pre<weird_label>(x > 0)
{
}

// Parenthesized comparison expression — result is bool, NOT an
// assertion_control_object, so this should fail the concept check.
void g(int x)
  pre<(weird_label_t{} < weird_label_t{})>(x > 0)  // { dg-error "class type" }
{
}

// Another form: comparison that produces bool.
void h(int x)
  pre<(weird_label > weird_label)>(x > 0)  // { dg-error "class type" }
{
}
