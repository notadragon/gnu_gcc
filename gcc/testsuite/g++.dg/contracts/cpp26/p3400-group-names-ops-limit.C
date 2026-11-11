// P3400 [support.contract.control.ident] and notadragon_wg21 DECISIONS.md
// E189 (a): a label whose group_names is a range too long to read within
// the constexpr operation limit is an error, with the evaluator's note of
// the limit, rather than a label silently in no group.  No header: the
// names are read as const char *, and nothing else here comes near the
// lowered limit.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce -fcontract-group-evaluation-semantic=safety:ignore -fconstexpr-ops-limit=1000" }

struct name_iterator {
  int i;
  constexpr const char *operator* () const { return "safety"; }
  constexpr name_iterator &operator++ () { ++i; return *this; }
  constexpr bool operator== (const name_iterator &) const = default;
};

struct many_names {
  constexpr name_iterator begin () const { return { 0 }; }
  constexpr name_iterator end () const { return { 100000 }; }
};

struct many_t {
  using assertion_control_object = many_t;
  static constexpr many_names group_names{};
};
constexpr many_t many{};

struct few_names {
  constexpr name_iterator begin () const { return { 0 }; }
  constexpr name_iterator end () const { return { 1 }; }
};

struct few_t {
  using assertion_control_object = few_t;
  static constexpr few_names group_names{};
};
constexpr few_t few{};

void f (int x) pre<many> (x > 0) {}		// { dg-error "'group_names' facet of assertion-control object 'many_t' must be usable in a constant expression" }
void g (int x) pre<few> (x > 0) {}

// { dg-error "evaluation operation count exceeds limit of 1000" "" { target *-*-* } 0 }
