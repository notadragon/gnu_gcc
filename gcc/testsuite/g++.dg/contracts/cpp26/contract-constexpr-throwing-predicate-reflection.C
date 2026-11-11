// PR128543's own shape: a reflection metafunction that throws
// std::meta::exception from a contract predicate in a consteval block is a
// contract violation ([basic.contract.eval]), reported with the exception's
// type and what () string.  GCC-1006: the compiler segfaulted.
// { dg-do compile { target c++26 } }
// { dg-require-effective-target exceptions_enabled }
// { dg-additional-options "-freflection -fcontracts -fcontract-evaluation-semantic=enforce" }

#include <meta>

consteval {
  contract_assert (is_base_of_type (^^int, {})); // { dg-error "contract predicate exited via an exception of type 'std::meta::exception' in constant expression; 'what\\(\\)': " }
}
