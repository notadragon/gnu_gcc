// P3400: Redeclaration sameness checking for assertion-control labels.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3400" }

struct label_a_t { using assertion_control_object = label_a_t; };
struct label_b_t { using assertion_control_object = label_b_t; };
constexpr label_a_t a{};
constexpr label_b_t b{};

// Same label on redeclaration — OK.
void same(int x) pre<a>(x > 0);
void same(int x) pre<a>(x > 0) { }

// Different labels — error.
void diff(int x) pre<a>(x > 0);
void diff(int x) pre<b>(x > 0) { }  // { dg-error "mismatched assertion-control label" }

// Label vs no label — error.
void one_label(int x) pre<a>(x > 0);
void one_label(int x) pre(x > 0) { }  // { dg-error "mismatched assertion-control label" }

// No label vs label — error.
void other_label(int x) pre(x > 0);
void other_label(int x) pre<a>(x > 0) { }  // { dg-error "mismatched assertion-control label" }
