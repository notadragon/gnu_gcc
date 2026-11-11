// The header is precompiled with a configuration file that makes every
// contract ignore, and this TU has none (enforce).  The PCH does not match,
// so it is not used: the header's configuration must not silently override
// the TU's.  pch.exp removes the header source for the compile that uses the
// PCH, so the rejection is the error below; reading the header as source
// instead diagnoses the violated precondition in constant evaluation.
//
// No Clang mirror: Clang's contracts take the TU's configuration.
// { dg-options "-std=c++26 -fcontracts" }

#include "contract-config-mismatch.H" // { dg-error "PCH files were found, but they were invalid" }
// { dg-prune-output "No such file or directory" }
// { dg-prune-output "compilation terminated" }

constexpr int v = g (-1);
