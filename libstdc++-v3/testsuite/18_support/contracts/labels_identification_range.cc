// { dg-options "-fcontracts-p3850" }
// { dg-do compile { target c++26 } }

// P3400 [support.contract.control.ident]: identification_label accepts any
// const range whose elements convert to string_view, and combining labels
// with operator| gives the sorted, unique union of the constituents' names
// ([support.contract.labels.combine]), whatever their range types.

#include <contracts>
#include <array>
#include <initializer_list>
#include <span>
#include <string_view>

namespace L = std::contracts::labels;
using L::identification_label;
using L::operator|;
using L::operator""group;

struct ptrs {
  using assertion_control_object = ptrs;
  static constexpr const char* const group_names[2]
    = { "b", "a_name_longer_than_a_pointer" };
};

struct std_array_sv {
  using assertion_control_object = std_array_sv;
  static constexpr std::array<std::string_view, 2> group_names = { "d", "c" };
};

struct sv_array {
  using assertion_control_object = sv_array;
  static constexpr std::string_view group_names[2] = { "c", "e" };
};

struct init_list {
  using assertion_control_object = init_list;
  static constexpr std::initializer_list<const char*> group_names
    = { "f", "b" };
};

constexpr const char* span_names[3] = { "h", "g", "h" };
struct span {
  using assertion_control_object = span;
  static constexpr std::span<const char* const> group_names{ span_names };
};

struct non_const {
  using assertion_control_object = non_const;
  std::array<std::string_view, 1> group_names = { "x" };
};

struct ints {
  using assertion_control_object = ints;
  static constexpr int group_names[1] = { 0 };
};

static_assert( identification_label<ptrs> );
static_assert( identification_label<std_array_sv> );
static_assert( identification_label<sv_array> );
static_assert( identification_label<init_list> );
static_assert( identification_label<span> );
static_assert( identification_label<L::assertion_group_label<2>> );
static_assert( !identification_label<non_const> );
static_assert( !identification_label<ints> );
static_assert( !identification_label<L::empty_label_t> );
static_assert( L::is_identification_label<span{}> );

template<typename R>
constexpr bool
names_are(const R& r, std::initializer_list<std::string_view> want)
{
  auto w = want.begin();
  for (std::string_view n : r)
    {
      if (w == want.end() || *w != n)
	return false;
      ++w;
    }
  return w == want.end();
}

constexpr auto all = ptrs{} | std_array_sv{} | sv_array{} | init_list{}
		     | span{} | "a"group;
static_assert( identification_label<decltype(all)> );
static_assert( names_are(all.group_names,
			 { "a", "a_name_longer_than_a_pointer", "b", "c", "d",
			   "e", "f", "g", "h" }) );

// One identification constituent: its names, sorted and unique.
constexpr auto one = L::review | span{};
static_assert( identification_label<decltype(one)> );
static_assert( names_are(one.group_names, { "g", "h" }) );

// No identification constituent: not an identification label.
static_assert( !identification_label<decltype(L::review | L::empty_label)> );
