// gcc-647-pch-redecl-contracts.h: the precompiled header.
int nf (int x) pre (x > 1);
int nf (int x);
int nf2 (int x) pre (x > 1);
