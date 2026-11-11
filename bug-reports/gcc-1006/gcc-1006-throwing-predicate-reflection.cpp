// GCC-1006: PR128543's own reproducer.  is_base_of_type throws
// std::meta::exception for a null reflection, from a contract predicate
// during constant evaluation.
//
//   g++ -std=c++26 -freflection -c gcc-1006-throwing-predicate-reflection.cpp

#include <meta>

consteval {
    contract_assert(is_base_of_type(^^int, {}));
}
