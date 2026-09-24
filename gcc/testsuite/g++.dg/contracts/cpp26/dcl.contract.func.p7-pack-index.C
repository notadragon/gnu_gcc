// N5008 dcl.contract.func/p7:
// If the predicate of a postcondition assertion of a function f odr-uses a
// non-reference parameter of f, that parameter shall have const type.
//
// A pack-index-expression (pack...[i]) odr-uses only the SELECTED element, so
// the requirement applies to that element alone -- not to the whole pack.  A
// fold expression odr-uses every element, so all must be const.  Instantiation
// is triggered by calls (rather than explicit instantiation) to avoid an
// unrelated explicit-instantiation matching bug with cv-qualified pack
// arguments.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts" }

// Pack indexing: selected element is non-const -> error.
template <typename... Ts>
void pi_nonconst (Ts... a) post (a...[0] == 0) {} // { dg-error "used in a postcondition must be const" }
void use_pi_nonconst () { pi_nonconst (1); }

// Pack indexing: selected element (index 1) is const, though a sibling is not
// -> OK.  Only the odr-used element is constrained.
template <typename... Ts>
void pi_const_sel (Ts... a) post (a...[1] == 0) {}
void use_pi_const_sel () { pi_const_sel<int, const int> (1, 2); }

// Pack indexing: selected element (index 0) is non-const while a sibling is
// const -> error (must not be masked by the const sibling).
template <typename... Ts>
void pi_nonconst_sel (Ts... a) post (a...[0] == 0) {} // { dg-error "used in a postcondition must be const" }
void use_pi_nonconst_sel () { pi_nonconst_sel<int, const int> (1, 2); }

// Whole-pack const (const Ts... a) makes every element const -> OK.
template <typename... Ts>
void pi_all_const (const Ts... a) post (a...[0] == 0) {}
void use_pi_all_const () { pi_all_const (1); }

// Fold over a non-const pack -> error (every element is odr-used).
template <typename... Ts>
void fold_nonconst (Ts... a) post ((... && (a == 0))) {} // { dg-error "used in a postcondition must be const" }
void use_fold_nonconst () { fold_nonconst (1); }

// Fold over a const pack -> OK.
template <typename... Ts>
void fold_const (const Ts... a) post ((... && (a == 0))) {}
void use_fold_const () { fold_const (1); }
