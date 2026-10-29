// Watch test for an open bug (GCC-899): identification_label and the
// compiler's reading of group_names are narrower than P3400 proposes.
// [support.contract.control.ident] requires group_names to be a const range
// whose elements convert to string_view.  The library concept instead
// requires group_names[0] to convert to const char*, and both the library
// (__group_count) and the compiler (ensure_contract_groups) count the names
// as sizeof (group_names) / sizeof (group_names[0]).  So a label whose
// names are string_views, or are held in an initializer_list, is not an
// identification label and its groups are ignored; and a span of names is
// accepted but only sizeof (span) / sizeof (char *) of them -- two -- are
// read.  When it is fixed, each xfailed dg-bogus line reports an XPASS.
//
// Each label puts "safety" last, and the safety group is ignored, so a
// violated precondition of a label whose groups are read is not evaluated
// during constant evaluation; one whose groups are missed is enforced and
// makes the call non-constant.
//
// Clang: libc++ in llvm_llvm-project has the same concept and the same
// sizeof-based count (no mirror test yet).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=enforce -fcontract-group-evaluation-semantic=safety:ignore" }

#include <contracts>
#include <array>
#include <initializer_list>
#include <span>
#include <string_view>

using std::contracts::labels::identification_label;

// Control: an array of pointers is accepted and read today.
struct ptr_array_t {
  using assertion_control_object = ptr_array_t;
  static constexpr const char *const group_names[2] = { "x", "safety" };
};

struct std_array_sv_t {
  using assertion_control_object = std_array_sv_t;
  static constexpr std::array<std::string_view, 2> group_names
    = { "x", "safety" };
};

struct sv_array_t {
  using assertion_control_object = sv_array_t;
  static constexpr std::string_view group_names[2] = { "x", "safety" };
};

struct init_list_t {
  using assertion_control_object = init_list_t;
  static constexpr std::initializer_list<const char *> group_names
    = { "x", "safety" };
};

constexpr const char *span_names[3] = { "x", "y", "safety" };
struct span_t {
  using assertion_control_object = span_t;
  static constexpr std::span<const char *const> group_names{ span_names };
};

static_assert (identification_label<ptr_array_t>);
static_assert (identification_label<std_array_sv_t>);	// { dg-bogus "static assertion failed" "GCC-899" { xfail *-*-* } }
static_assert (identification_label<sv_array_t>);	// { dg-bogus "static assertion failed" "GCC-899" { xfail *-*-* } }
static_assert (identification_label<init_list_t>);	// { dg-bogus "static assertion failed" "GCC-899" { xfail *-*-* } }
static_assert (identification_label<span_t>);

constexpr int a (int x) pre<ptr_array_t{}> (x > 0) { return x; }
constexpr int b (int x) pre<std_array_sv_t{}> (x > 0) { return x; }	// { dg-bogus "contract predicate is false" "GCC-899" { xfail *-*-* } }
constexpr int c (int x) pre<sv_array_t{}> (x > 0) { return x; }	// { dg-bogus "contract predicate is false" "GCC-899" { xfail *-*-* } }
constexpr int d (int x) pre<init_list_t{}> (x > 0) { return x; }	// { dg-bogus "contract predicate is false" "GCC-899" { xfail *-*-* } }
constexpr int e (int x) pre<span_t{}> (x > 0) { return x; }	// { dg-bogus "contract predicate is false" "GCC-899" { xfail *-*-* } }

static_assert (a (-1) == -1);
static_assert (b (-1) == -1);
static_assert (c (-1) == -1);
static_assert (d (-1) == -1);
static_assert (e (-1) == -1);
