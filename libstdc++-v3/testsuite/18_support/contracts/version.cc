// { dg-options "-fcontracts-p3850" }
// { dg-do preprocess { target c++26 } }
// { dg-require-effective-target hosted }
// { dg-add-options no_pch }

// <version> defines every contracts feature-test macro, with its value
// (GCC-194).  feature_test_macros.cc checks the same macros through
// <contracts> and <cassert>.

#include <version>

#ifndef __cpp_lib_contracts
# error "Feature-test macro __cpp_lib_contracts missing in <version>"
#elif __cpp_lib_contracts != 202502L
# error "Feature-test macro __cpp_lib_contracts has wrong value in <version>"
#endif
#ifndef __cpp_lib_contracts_message
# error "Feature-test macro __cpp_lib_contracts_message missing in <version>"
#elif __cpp_lib_contracts_message <= 0
# error "Feature-test macro __cpp_lib_contracts_message has wrong value in <version>"
#endif
#ifndef __cpp_lib_contracts_api
# error "Feature-test macro __cpp_lib_contracts_api missing in <version>"
#elif __cpp_lib_contracts_api <= 0
# error "Feature-test macro __cpp_lib_contracts_api has wrong value in <version>"
#endif
#ifndef __cpp_lib_assert_can_use_contracts
# error "Feature-test macro __cpp_lib_assert_can_use_contracts missing in <version>"
#elif __cpp_lib_assert_can_use_contracts <= 0
# error "Feature-test macro __cpp_lib_assert_can_use_contracts has wrong value in <version>"
#endif
#ifndef __cpp_lib_contracts_implicit
# error "Feature-test macro __cpp_lib_contracts_implicit missing in <version>"
#elif __cpp_lib_contracts_implicit <= 0
# error "Feature-test macro __cpp_lib_contracts_implicit has wrong value in <version>"
#endif
#ifndef __cpp_lib_contracts_labels
# error "Feature-test macro __cpp_lib_contracts_labels missing in <version>"
#elif __cpp_lib_contracts_labels <= 0
# error "Feature-test macro __cpp_lib_contracts_labels has wrong value in <version>"
#endif
#ifndef __cpp_lib_contracts_report
# error "Feature-test macro __cpp_lib_contracts_report missing in <version>"
#elif __cpp_lib_contracts_report <= 0
# error "Feature-test macro __cpp_lib_contracts_report has wrong value in <version>"
#endif
#ifndef __cpp_lib_replaceable_contract_violation_handler
# error "Feature-test macro __cpp_lib_replaceable_contract_violation_handler missing in <version>"
#elif __cpp_lib_replaceable_contract_violation_handler != 202603L
# error "Feature-test macro __cpp_lib_replaceable_contract_violation_handler has wrong value in <version>"
#endif
