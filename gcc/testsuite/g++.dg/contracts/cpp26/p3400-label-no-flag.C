// P3400: Label syntax rejected without -fcontracts-p3400.
// { dg-do compile { target c++26 } }

struct my_label_t {
  using assertion_control_object = my_label_t;
};
constexpr my_label_t my_label{};

void f(int x)
  pre<my_label>(x > 0)  // { dg-error "assertion-control labels require" }
{
}
