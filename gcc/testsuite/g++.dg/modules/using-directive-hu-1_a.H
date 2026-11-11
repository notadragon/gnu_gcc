// D3400R6 [module.interface]: every using-directive in a header unit is
// exported, whether at global scope or nested in a namespace, so an
// importer's unqualified and qualified lookup see the nominated namespace
// (using-directive-hu-1_b.C).

// { dg-additional-options "-fmodule-header" }
// { dg-module-cmi {} }

namespace hu1_n
{
  inline int hu1_v = 1;
}
using namespace hu1_n;

namespace hu1_a
{
  namespace hu1_m
  {
    inline int hu1_w = 2;
  }
  using namespace hu1_m;
}
