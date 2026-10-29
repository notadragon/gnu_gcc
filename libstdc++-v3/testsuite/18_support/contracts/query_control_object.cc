// { dg-options "-fcontracts-p3850 -fcontract-evaluation-semantic=observe" }
// { dg-do run { target c++26 } }

// P3400 contract_violation::query_control_object().  With a queryable label
// the key maps to the label's value; with no label at all it must return
// null rather than misbehave.  Neither had libstdc++ coverage.

#include <contracts>
#include <testsuite_hooks.h>

using std::contracts::contract_violation;

constexpr int key1 = 1;

struct queryable_t
{
  using assertion_control_object = queryable_t;
  void* query(const void* __key, std::size_t __index) const
  {
    if (__index != 0)
      return nullptr;
    if (__key == &key1)
      return const_cast<char*>("hit");
    return nullptr;
  }
};
constexpr queryable_t qlabel{};

static int labelled_calls = 0;
static int plain_calls = 0;
static bool in_labelled = false;

void handle_contract_violation(const contract_violation& v)
{
  if (in_labelled)
    {
      ++labelled_calls;
      VERIFY( v.query_control_object(&key1) != nullptr );
      // An unknown key, and an out-of-range index for a known one.
      static const int other = 0;
      VERIFY( v.query_control_object(&other) == nullptr );
      VERIFY( v.query_control_object(&key1, 1) == nullptr );
    }
  else
    {
      ++plain_calls;
      // No label at all: nothing to query.
      VERIFY( v.query_control_object(&key1) == nullptr );
    }
}

void labelled(int i) pre<qlabel> (i > 10) { }
void plain(int i) pre (i > 10) { }

int main()
{
  in_labelled = true;
  labelled(0);
  in_labelled = false;
  plain(0);

  VERIFY( labelled_calls == 1 );
  VERIFY( plain_calls == 1 );
}
