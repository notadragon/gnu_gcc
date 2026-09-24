// A P3400 assertion-control label on a contract that follows a TRAILING return
// type must be parsed as a contract, not consumed as a template-id
// abstract-declarator.  The trailing return type's type-specifier-seq stops at
// the contract correctly; the hazard is the next step, where
// cp_parser_type_id_1's tentative abstract-declarator parse can swallow
// "pre<lbl>" (a labelled contract looks like a template-id), dropping the
// contract and failing the declaration with a spurious "expected initializer".
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3850 -fcontracts-p3400" }

struct L { using assertion_control_object = L; };
constexpr L lbl{};

// Trailing-return-type function with a labelled precondition and postcondition.
auto f (int a) -> int pre<lbl> (a > 0) post<lbl> (r : r > 0) { return a; }

// Leading-return control (already worked); the two forms must agree.
int g (int a) pre<lbl> (a > 0);

// Bare (unlabelled) contract on a trailing return also still works.
auto h (int a) -> int pre (a > 0) { return a; }
