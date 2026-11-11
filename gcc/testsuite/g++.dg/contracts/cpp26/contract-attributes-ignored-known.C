// A standard attribute GCC knows (as opposed to the misspelled one in
// contract-assert-warn-attributes.C) is ignored on every kind of contract
// assertion with the same single warning, and without a second warning from
// the attribute's own handling, even under -Wall (GCC-192).
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -Wall" }

int foo (int x)
  pre [[unlikely]] (x > 41)  // { dg-warning "attributes are ignored on function contract specifiers" }
  post [[likely]] (r: r > 0)  // { dg-warning "attributes are ignored on function contract specifiers" }
{
  contract_assert [[unlikely]] (x > 0);  // { dg-warning "attributes are ignored on 'contract_assert'" }
  return x + 1;
}
