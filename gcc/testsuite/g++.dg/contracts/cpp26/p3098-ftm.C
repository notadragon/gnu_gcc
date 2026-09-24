// P3098: the postcondition-captures feature-test macro is defined with the flag.
// (Language feature only; there is no library __cpp_lib_ counterpart.)
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts-p3098" }

#ifndef __cpp_contracts_postcondition_captures
#error "__cpp_contracts_postcondition_captures not defined"
#endif

#if __cpp_contracts_postcondition_captures != 202606L
#error "__cpp_contracts_postcondition_captures has wrong value"
#endif
