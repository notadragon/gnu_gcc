// Extra translation unit of refinable-fork-label.C, linked first: the
// definitions the linker keeps.

#include <contracts>

using std::contracts::evaluation_semantic;
using std::contracts::evaluation_semantic_set;
using enum evaluation_semantic;

template<evaluation_semantic... S>
struct allow {
  using assertion_control_object = allow;
  static constexpr evaluation_semantic_set allowed_semantics{S...};
};

inline constexpr allow<enforce, observe> l_open{};
inline constexpr allow<enforce, quick_enforce> l_mixed{};
inline constexpr allow<quick_enforce, noexcept_enforce, assume> l_settled{};
inline constexpr allow<enforce> l_enforce{};

[[gnu::noinline]] inline int
u_open (int x)
{
  contract_assert<l_open> (x > 0);	// observe
  return x;
}

[[gnu::noinline]] inline int
u_mixed (int x)
{
  contract_assert<l_mixed> (x > 0);	// enforce
  return x;
}

[[gnu::noinline]] inline int
s_settled (int x)
{
  contract_assert<l_settled> (x > 0);	// noexcept_enforce
  return x;
}

[[gnu::noinline]] inline int
s_mixed_nx (int x) noexcept
{
  contract_assert<l_mixed> (x > 0);	// enforce
  return x;
}

[[gnu::noinline]] inline int
s_enforce_only (int x)
{
  contract_assert<l_enforce> (x > 0);	// enforce
  return x;
}

int (*volatile keep[]) (int) = {
  u_open,
  u_mixed,
  s_settled,
  s_mixed_nx,
  s_enforce_only,
};
