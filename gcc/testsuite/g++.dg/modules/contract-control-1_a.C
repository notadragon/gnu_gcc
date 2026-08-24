// P3400: a module must be able to export a contract-control
// using-directive.
//
// Regression test: contract-control using-directives are held in a vector
// of their own (cp_binding_level::contract_control_usings), because the
// names they introduce are visible only inside assertion-control
// expressions.  write_using_directives and read_using_directives walked
// only the ordinary using_directives vector, so the contract-control ones
// were never streamed into a CMI at all.  No module could export one, and
// "export using contract_control namespace X;" was accepted with no
// diagnostic and silently discarded.

// { dg-additional-options "-fmodules -fcontracts -fcontracts-p3400" }
// { dg-module-cmi g9lbl }

export module g9lbl;

export namespace g9ns
{
  struct my_label_t
  {
    using assertion_control_object = my_label_t;
  };
  inline constexpr my_label_t my_label{};
}

export using contract_control namespace g9ns;
