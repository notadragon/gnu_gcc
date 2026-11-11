// A dependent discarded comma operand in a postcondition must not crash the
// compiler.
//
// walk_discarded_operand must establish that e has a type before testing
// VOID_TYPE_P (TREE_TYPE (e)).  A postcondition's result binding is a
// PARM_DECL of type auto until the return type is deduced, so the predicate is
// type-dependent while it is being parsed, and a dependent member access, call,
// subscript or ?: is built by build_min_nt -- which leaves TREE_TYPE null.
// VOID_TYPE_P is TREE_CODE (x) == VOID_TYPE, so an unguarded test segfaults.
//
// The shapes at risk are exactly the ones build_min_nt produces.
// Dependent unary and binary operators do carry a (dependent) type, which is
// why `(r.id, true)` reaches the null and `(r.id + 0, true)` does not -- the
// class type and the member access are incidental, and `(g (r), true)` over a
// scalar result reaches it just as well.
//
// This walk implements [dcl.contract.func]/7 over the finished predicate; see
// pr126897.C for the rule itself.  The cases below pin both that these compile
// and that the rule still reaches what it should once past them.
//
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -Wno-unused-value" }

struct Inner { int x; };

struct Plain
{
  int id;
  int arr[4];
  Inner in;
  int get () const { return id; }
};

int use (Plain);
int use_scalar (int);

// The shapes that reach the null type.  None of them names a parameter of f,
// so [dcl.contract.func]/7 has nothing to say and all are well-formed.

Plain member_access (int b) post (r : (r.id, true)) { return Plain{b}; }

Plain nested_member (int b) post (r : (r.in.x, true)) { return Plain{b}; }

Plain member_call (int b) post (r : (r.get (), true)) { return Plain{b}; }

Plain subscript (int b) post (r : (r.arr[0], true)) { return Plain{b}; }

Plain free_call (int b) post (r : (use (r), true)) { return Plain{b}; }

// A scalar result reaches the same crash through a dependent CALL_EXPR, which
// is what shows the class type was never the ingredient.
int scalar_call (int b) post (r : (use_scalar (r), true)) { return b; }

Plain *via_arrow (int b) post (r : (r->id, true)) { return nullptr; }

auto deduced_return (int b) post (r : (r.id, true)) { return Plain{b}; }

// A declaration with no definition takes the same grok_contract path.
Plain declared_only (int b) post (r : (r.id, true));

// A dependent COND_EXPR is built by build_min_nt too.  Its condition is
// evaluated, so a non-const parameter named there IS odr-used and must be
// diagnosed -- and the null type is reached before the walk gets that far.
Plain cond_const_param (int const b) post (r : (b ? r.id : r.in.x, true))
{ return Plain{b}; }

Plain cond_plain_param (int b) post (r : (b ? r.id : r.in.x, true)) // { dg-error "must be const" }
{ return Plain{b}; }

// A lambda instantiated as part of a template reaches the same code through
// cp_parser_late_contract_condition rather than grok_contract.
template <class T> Plain in_template (T a)
{
  auto inner = [] (int b) post (r : (r.id, true)) { return Plain{b}; };
  return inner ((int) a);
}
Plain instantiate_it () { return in_template (1); }

// Repeating a contract on both declarations of a member function is
// ill-formed, [dcl.contract.func]/6.  It segfaulted instead of saying so,
// because the redeclaration check must walk the predicate before it can
// report the mismatch.
struct Redeclared { Plain f (int b); };
Plain Redeclared::f (int b) post (r : (r.id, true)) // { dg-error "declaration adds contracts" }
{ return Plain{b}; }

// CONTROLS -- these never crashed, and must stay well-formed.

// The result binding alone, scalar and class-typed: a VIEW_CONVERT_EXPR (the
// contract const wrapper) around a typed decl.
int bare_scalar_result (int b) post (r : (r, true)) { return b; }
Plain bare_class_result (int b) post (r : (r, true)) { return Plain{b}; }

// Dependent operators keep a type, so these took the typed path all along.
int scalar_arith (int b) post (r : (r + 0, true)) { return b; }
int scalar_negate (int b) post (r : (-r, true)) { return b; }
Plain member_arith (int b) post (r : (r.id + 0, true)) { return Plain{b}; }
Plain cast_void (int b) post (r : ((void) r.id, true)) { return Plain{b}; }

// A member function contracted on its first declaration, and a static one:
// the late-parse path, which was already reaching typed trees.
struct FirstDecl
{
  Plain f (int b) post (r : (r.id, true));
  static Plain g (int b) post (r : (r.id, true));
};
Plain FirstDecl::f (int b) { return Plain{b}; }
Plain FirstDecl::g (int b) { return Plain{b}; }

// The rule the walk exists for must still fire correctly on either side of the
// comma, once past the null-typed operand.  A non-const parameter
// that is the discarded operand's potential result is exempt; one that is an
// argument to a call there is not.
Plain exempt_param (int b) post (r : (b, r.id != 0)) { return Plain{b}; }

Plain odr_used_param (int b) post (r : (use_scalar (b), true)) // { dg-error "must be const" }
{ return Plain{b}; }
