// D4405 ("Taking the Address of a Function"): a parenthesized function
// name takes its address just as the name does -- bound to a reference,
// converted to a pointer, the operand of the unary &, an argument, deduced
// by auto or decltype(auto), however many parentheses, in a template's
// instantiation and in a default argument -- so an always_prove or
// always_diagnose precondition cannot be proven for a call through the
// address.  A parenthesized qualified name, template-id and closure are
// diagnosed too; a parenthesized direct call is not taking the address
// (GCC-936).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontracts-p4405" }
// { dg-skip-if "requires hosted libstdc++ for stdc++exp" { ! hostedlib } }

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;

struct always_prove_t {
  using assertion_control_object = always_prove_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::prove};
};
constexpr always_prove_t always_prove{};

struct always_diagnose_t {
  using assertion_control_object = always_diagnose_t;
  static constexpr evaluation_semantic_set
    allowed_semantics{evaluation_semantic::diagnose};
};
constexpr always_diagnose_t always_diagnose{};

void fp (int x) pre<always_prove> (x > 0);	// { dg-message "precondition here" }
void fd (int x) pre<always_diagnose> (x > 0);	// { dg-message "precondition here" }
void fn (int x) pre (x > 0);

struct S {
  static void sp (int x) pre<always_prove> (x > 0);	// { dg-message "precondition here" }
};

template<typename T>
void tp (T x) pre<always_prove> (x > 0);	// { dg-message "precondition here" }

void take (void (*) (int));
void take_ref (void (&) (int));

void (&gr) (int) = (fp);			// { dg-error "contract assertion cannot be proven for a call through the address of 'void fp\\(int\\)'" }

void use (S &s)
{
  void (&r1) (int) = (fp);			// { dg-error "contract assertion cannot be proven for a call through the address of 'void fp\\(int\\)'" }
  void (&r2) (int) = ((fp));			// { dg-error "cannot be proven for a call through the address" }
  void (&&r3) (int) = (fp);			// { dg-error "cannot be proven for a call through the address" }
  void (*p1) (int) = (fp);			// { dg-error "cannot be proven for a call through the address" }
  void (*p2) (int) = ((fp));			// { dg-error "cannot be proven for a call through the address" }
  void (*p3) (int) = &(fp);			// { dg-error "cannot be proven for a call through the address" }
  void (*p4) (int) = &((fp));			// { dg-error "cannot be proven for a call through the address" }
  void (*p5) (int) = (&fp);			// { dg-error "cannot be proven for a call through the address" }
  take ((fp));					// { dg-error "cannot be proven for a call through the address" }
  take_ref ((fp));				// { dg-error "cannot be proven for a call through the address" }
  auto p6 = (fp);				// { dg-error "cannot be proven for a call through the address" }
  auto &r4 = (fp);				// { dg-error "cannot be proven for a call through the address" }
  decltype (auto) r5 = (fp);			// { dg-error "cannot be proven for a call through the address" }
  void (&r6) (int) = (fd);			// { dg-warning "contract assertion cannot be proven for a call through the address of 'void fd\\(int\\)' \\\[-Wcontract-unproven\\\]" }
  void (*p7) (int) = &(fd);			// { dg-warning "cannot be proven for a call through the address" }
  void (&r7) (int) = (fn);			// { dg-bogus "address|analyze" }
  void (*p8) (int) = (S::sp);			// { dg-error "cannot be proven for a call through the address" }
  void (&r8) (int) = (s.sp);			// { dg-error "cannot be proven for a call through the address" }
  void (*p9) (int) = (tp<int>);			// { dg-error "cannot be proven for a call through the address" }
  auto l = [] (int x) pre<always_prove> (x > 0) { return x; };	// { dg-message "precondition here" }
  int (*p10) (int) = (l);			// { dg-error "cannot be proven for a call through the address" }
  int (*p11) (int) = ((l));			// { dg-error "cannot be proven for a call through the address" }
  decltype (&(fp)) q = nullptr;		// { dg-bogus "address" }
  (void) sizeof (&(fp));			// { dg-bogus "address" }
  (fp) (1);					// { dg-bogus "address|cannot be proven" }
  ((fp)) (1);					// { dg-bogus "address|cannot be proven" }
  (void) r1; (void) r2; (void) r3; (void) p1; (void) p2; (void) p3;
  (void) p4; (void) p5; (void) p6; (void) r4; (void) r5; (void) r6;
  (void) p7; (void) r7; (void) p8; (void) r8; (void) p9; (void) p10;
  (void) p11; (void) q;
}

template<typename T>
void inst (T)
{
  void (&r) (int) = (fp);			// { dg-error "cannot be proven for a call through the address" }
  void (*p) (int) = &(fp);			// { dg-error "cannot be proven for a call through the address" }
  (void) r; (void) p;
}

template void inst (int);

void dflt (void (&) (int) = (fp));		// { dg-error "cannot be proven for a call through the address" }
