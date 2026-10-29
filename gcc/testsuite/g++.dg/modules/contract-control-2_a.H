// P3400: a header unit carries a contract-control using-directive to its
// importers, as a named module exports one (contract-control-1_a.C): the
// same streaming of cp_binding_level::contract_control_usings applies.
// (GCC-203.)

// { dg-additional-options "-fmodule-header -fcontracts -fcontracts-p3400" }
// { dg-module-cmi {} }

namespace g10ns
{
  struct my_label_t
  {
    using assertion_control_object = my_label_t;
  };
  inline constexpr my_label_t my_label{};
}

using contract_control namespace g10ns;
