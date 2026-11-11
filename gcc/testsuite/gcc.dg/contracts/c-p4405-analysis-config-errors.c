/* D4405 in C: the configuration errors of the "analysis" match key, which
   the C and C++ front ends share -- it cannot appear with "caller", and an
   entry with it must have a semantic of ignore, diagnose or prove and no
   dynamic selector.  Nor can an entry for constant evaluation alone
   ("constexpr": true) select prove or diagnose (DECISIONS.md E142).  */
/* { dg-do compile } */
/* { dg-options "-fcontracts-p4299 -fcontracts-p4405" } */
/* { dg-additional-options "-fcontract-configuration-file=${srcdir}/gcc.dg/contracts/c-p4405-analysis-config-errors.json" } */
/* { dg-regexp {[^\n]*c-p4405-analysis-config-errors\.json: In JSON value '/0/match/analysis'} } */
/* { dg-regexp {[^\n]*c-p4405-analysis-config-errors\.json:[0-9]+:[0-9]+: error: 'analysis' and 'caller' cannot both appear in 'match'[^\n]*} } */
/* { dg-regexp {[^\n]*c-p4405-analysis-config-errors\.json: In JSON value '/1/output/semantic'} } */
/* { dg-regexp {[^\n]*c-p4405-analysis-config-errors\.json:[0-9]+:[0-9]+: error: the 'semantic' of an entry matching 'analysis' must be 'ignore', 'diagnose' or 'prove'[^\n]*} } */
/* { dg-regexp {[^\n]*c-p4405-analysis-config-errors\.json: In JSON object '/2/output/dynamic'} } */
/* { dg-regexp {[^\n]*c-p4405-analysis-config-errors\.json:[0-9]+:[0-9]+: error: an entry matching 'analysis' cannot have a 'dynamic' output[^\n]*} } */
/* { dg-regexp {[^\n]*c-p4405-analysis-config-errors\.json: In JSON value '/3/output/semantic'} } */
/* { dg-regexp {[^\n]*c-p4405-analysis-config-errors\.json:[0-9]+:[0-9]+: error: the 'semantic' of an entry matching 'constexpr' cannot be 'prove' or 'diagnose'[^\n]*} } */

int f (int x) { return x; }
