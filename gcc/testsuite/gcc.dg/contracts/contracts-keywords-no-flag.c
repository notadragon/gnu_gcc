/* Without -fcontracts-p4299, _Pre/_Post/_ContractAssert are identifiers.  */
/* { dg-do compile } */

int _Pre = 1;
int _Post = 2;
int _ContractAssert = 3;

int use(void)
{
  return _Pre + _Post + _ContractAssert;
}
