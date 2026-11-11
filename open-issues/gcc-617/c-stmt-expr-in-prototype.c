/* GCC-617: a statement expression in a prototype contract is rejected.
   gcc -fcontracts-p4299 -fsyntax-only
   Expected: no diagnostic.  Actual: "braced-group within expression allowed
   only inside a function".  */
int h (int x) _Pre (__extension__ ({ int t = x; t > 0; }));
