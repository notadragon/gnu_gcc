// P3400 [support.contract.control.ident]: a label's group names are read in
// full, however long, with every byte value, and up to the 4096 names a
// label may hold (p3400-group-names-too-many.C).  GCC has no cap of its
// own on the length of a name (a name is read in one constexpr loop, so
// -fconstexpr-loop-limit bounds it), and none below 4096 on the number of
// names: a name longer than 256 characters selects its own group and not
// the group of its first 256 characters; a name holding the bytes 0xFE or
// 0xFF is not the name without them, and a later name in the same range
// is still read; a newline does not split a name; and a range of 255 (or
// 300) names is read to its last name.  Each configured group maps to
// ignore and everything else is enforced, so an assertion that selects the
// wrong group is an error during constant evaluation.
//
// Mirror: tests/tests/contracts/p3400groups/read_errors.sft.cpp in the
// edgcpp_compiler fork.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce -fcontract-group-evaluation-semantic=safety:ignore" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx:ignore" }
// { dg-additional-options "-fcontract-group-evaluation-semantic=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxzz:ignore" }

// A name of 256 x followed by TAIL (TAIL 0: just the 256 x).
struct long_name {
  char s[259];
  constexpr long_name (char t1, char t2) : s{}
  {
    for (int i = 0; i < 256; ++i)
      s[i] = 'x';
    s[256] = t1;
    s[257] = t2;
  }
};
constexpr long_name x256{0, 0}, x256y{'y', 0}, x256zz{'z', 'z'};

template <const char *N> struct one_name_t {
  using assertion_control_object = one_name_t;
  static constexpr const char *group_names[1] = { N };
};

// 256 x: configured, so ignored.
constexpr one_name_t<x256.s> l_x256{};
constexpr int
f_x256 (int x)
{
  contract_assert<l_x256> (x > 0);
  return 1;
}
static_assert (f_x256 (-1));

// 256 x then zz, 258 characters: configured on its own, so ignored.
constexpr one_name_t<x256zz.s> l_x256zz{};
constexpr int
f_x256zz (int x)
{
  contract_assert<l_x256zz> (x > 0);
  return 1;
}
static_assert (f_x256zz (-1));

// 256 x then y: not the 256-x group, and not configured, so enforced.
constexpr one_name_t<x256y.s> l_x256y{};
constexpr int
f_x256y (int x)
{
  contract_assert<l_x256y> (x > 0);	// { dg-error "contract predicate is false" }
  return 1;
}
static_assert (f_x256y (-1));

// Every byte value is part of a name: "safety\xfe" and "safety\xff" are
// not "safety", so enforced.
constexpr char safety_fe[] = "safety\xfe";
constexpr char safety_ff[] = "safety\xff";
constexpr char safety[] = "safety";

constexpr one_name_t<safety_fe> l_fe{};
constexpr int
f_fe (int x)
{
  contract_assert<l_fe> (x > 0);	// { dg-error "contract predicate is false" }
  return 1;
}
static_assert (f_fe (-1));

constexpr one_name_t<safety_ff> l_ff{};
constexpr int
f_ff (int x)
{
  contract_assert<l_ff> (x > 0);	// { dg-error "contract predicate is false" }
  return 1;
}
static_assert (f_ff (-1));

// A 0xFF name does not end the range: "safety" after it selects the
// configured group, so ignored.
struct ff_then_safety_t {
  using assertion_control_object = ff_then_safety_t;
  static constexpr const char *group_names[2] = { safety_ff, safety };
};
constexpr ff_then_safety_t l_ff_safety{};
constexpr int
f_ff_safety (int x)
{
  contract_assert<l_ff_safety> (x > 0);
  return 1;
}
static_assert (f_ff_safety (-1));

// A newline inside a name does not split it: "x\nsafety" is not "safety",
// so enforced.
struct newline_t {
  using assertion_control_object = newline_t;
  static constexpr const char *group_names[1] = { "x\nsafety" };
};
constexpr newline_t l_newline{};
constexpr int
f_newline (int x)
{
  contract_assert<l_newline> (x > 0);	// { dg-error "contract predicate is false" }
  return 1;
}
static_assert (f_newline (-1));

// N names, only the last "safety": read in full, so ignored.
struct name_iterator {
  int i, last;
  constexpr const char *operator* () const { return i == last ? "safety" : "x"; }
  constexpr name_iterator &operator++ () { ++i; return *this; }
  constexpr bool operator== (const name_iterator &) const = default;
};

template <int N> struct n_names {
  constexpr name_iterator begin () const { return { 0, N - 1 }; }
  constexpr name_iterator end () const { return { N, N - 1 }; }
};

template <int N> struct n_t {
  using assertion_control_object = n_t;
  static constexpr n_names<N> group_names{};
};

template <int N> constexpr n_t<N> l_n{};

template <int N>
constexpr int
f_n (int x)
{
  contract_assert<l_n<N>> (x > 0);
  return 1;
}
static_assert (f_n<254> (-1));
static_assert (f_n<255> (-1));
static_assert (f_n<256> (-1));
static_assert (f_n<300> (-1));

// Control: the same range with no "safety" is enforced.
struct no_safety_iterator {
  int i;
  constexpr const char *operator* () const { return "x"; }
  constexpr no_safety_iterator &operator++ () { ++i; return *this; }
  constexpr bool operator== (const no_safety_iterator &) const = default;
};
struct no_safety_names {
  constexpr no_safety_iterator begin () const { return { 0 }; }
  constexpr no_safety_iterator end () const { return { 255 }; }
};
struct no_safety_t {
  using assertion_control_object = no_safety_t;
  static constexpr no_safety_names group_names{};
};
constexpr no_safety_t l_no_safety{};
constexpr int
f_no_safety (int x)
{
  contract_assert<l_no_safety> (x > 0);	// { dg-error "contract predicate is false" }
  return 1;
}
static_assert (f_no_safety (-1));
