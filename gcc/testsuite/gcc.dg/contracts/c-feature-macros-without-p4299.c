/* Without -fcontracts-p4299, __gcc_contracts_p4299 is not predefined
   (c-feature-macros.c).  */
/* { dg-do compile } */

#ifdef __gcc_contracts_p4299
#error "__gcc_contracts_p4299"
#endif
