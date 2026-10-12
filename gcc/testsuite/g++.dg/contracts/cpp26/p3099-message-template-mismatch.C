// P3099: a redeclaration of a template whose message is still as written
// is compared as an expression.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3099" }

struct M { constexpr unsigned long size () const { return 3; }
	   constexpr const char *data () const { return "abc"; } };
struct N { constexpr unsigned long size () const { return 2; }
	   constexpr const char *data () const { return "xy"; } };

template <class T> void f (int x) pre (x > 0, T{});
template <class T> void f (int x) pre (x > 0, M{}) {}	// { dg-error "mismatched contract diagnostic message" }

template <class T> void g (int x) pre (x > 0, T{});
template <class T> void g (int x) pre (x > 0, T{}) {}
