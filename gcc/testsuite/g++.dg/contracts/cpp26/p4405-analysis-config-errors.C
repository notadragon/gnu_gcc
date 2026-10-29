// D4405 ("P3595R1 Analysis Evaluation Configuration"): the configuration
// errors of the "analysis" match key.  It must be a boolean; it cannot
// appear in a match with "caller" (whatever "caller" says); and an entry
// with "analysis": true must have an output semantic of ignore, diagnose
// or prove, and no dynamic selector.  "analysis": false is the same as
// leaving the key out, so it may appear with "caller", and the key itself
// is known (no unknown-key warning).  Each erroneous entry is skipped.
// { dg-do compile { target c++26 } }
// { dg-additional-options "-fcontracts -fcontracts-p4405" }
// { dg-additional-options "-fcontract-configuration-file=${srcdir}/g++.dg/contracts/cpp26/p4405-analysis-config-errors.json" }
//
// The diagnostics point into the JSON file, so match them with dg-regexp.
// { dg-regexp {[^\n]*p4405-analysis-config-errors\.json: In JSON value '/0/match/analysis'} }
// { dg-regexp {[^\n]*p4405-analysis-config-errors\.json:[0-9]+:[0-9]+: error: 'analysis' must be a boolean[^\n]*} }
// { dg-regexp {[^\n]*p4405-analysis-config-errors\.json: In JSON value '/1/match/analysis'} }
// { dg-regexp {[^\n]*p4405-analysis-config-errors\.json:[0-9]+:[0-9]+: error: 'analysis' and 'caller' cannot both appear in 'match'[^\n]*} }
// { dg-regexp {[^\n]*p4405-analysis-config-errors\.json: In JSON value '/2/match/analysis'} }
// { dg-regexp {[^\n]*p4405-analysis-config-errors\.json:[0-9]+:[0-9]+: error: 'analysis' and 'caller' cannot both appear in 'match'[^\n]*} }
// { dg-regexp {[^\n]*p4405-analysis-config-errors\.json: In JSON object '/3/output/dynamic'} }
// { dg-regexp {[^\n]*p4405-analysis-config-errors\.json:[0-9]+:[0-9]+: error: an entry matching 'analysis' cannot have a 'dynamic' output[^\n]*} }
// { dg-regexp {[^\n]*p4405-analysis-config-errors\.json: In JSON value '/4/output/semantic'} }
// { dg-regexp {[^\n]*p4405-analysis-config-errors\.json:[0-9]+:[0-9]+: error: the 'semantic' of an entry matching 'analysis' must be 'ignore', 'diagnose' or 'prove'[^\n]*} }
// { dg-regexp {[^\n]*p4405-analysis-config-errors\.json: In JSON value '/5/output/semantic'} }
// { dg-regexp {[^\n]*p4405-analysis-config-errors\.json:[0-9]+:[0-9]+: error: the 'semantic' of an entry matching 'analysis' must be 'ignore', 'diagnose' or 'prove'[^\n]*} }

int f (int x) { return x; }
