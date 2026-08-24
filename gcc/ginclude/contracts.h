// C compatibility header <contracts.h> for C contracts (D4299)

// Copyright The GNU Toolchain Authors.
//
// This file is part of GCC.
//
// GCC is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// GCC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

#ifndef _CONTRACTS_H
#define _CONTRACTS_H

#ifdef __cplusplus
extern "C" {
#endif

// Opaque violation type.  A const contract_violation_t* always points
// to a std::contracts::contract_violation object constructed by the
// runtime.  The struct is never defined in any C-visible header.
typedef struct contract_violation_t contract_violation_t;

// Accessor functions
const char*
stdc_contract_violation_comment(const contract_violation_t*);
const char*
stdc_contract_violation_file(const contract_violation_t*);
const char*
stdc_contract_violation_function(const contract_violation_t*);
unsigned
stdc_contract_violation_line(const contract_violation_t*);
unsigned
stdc_contract_violation_column(const contract_violation_t*);
int
stdc_contract_violation_kind(const contract_violation_t*);
int
stdc_contract_violation_semantic(const contract_violation_t*);
int
stdc_contract_violation_detection_mode(const contract_violation_t*);
int
stdc_contract_violation_is_terminating(const contract_violation_t*);

// Assertion kind constants
#define STDC_CONTRACT_PRE            0x01
#define STDC_CONTRACT_POST           0x02
#define STDC_CONTRACT_ASSERT         0x03
#define STDC_CONTRACT_MANUAL         0x04
#define STDC_CONTRACT_CASSERT        0x05
// POST_CAPTURE and IMPLICIT do not arise from C contracts, but a shared
// handler in a mixed C/C++ program can receive them from the C++ side.
#define STDC_CONTRACT_POST_CAPTURE   0x06
#define STDC_CONTRACT_IMPLICIT       0x07

// Evaluation semantic constants
#define STDC_CONTRACT_IGNORE         0x01
#define STDC_CONTRACT_OBSERVE        0x02
#define STDC_CONTRACT_ENFORCE        0x03
#define STDC_CONTRACT_QUICK_ENFORCE  0x04

// Detection mode constants
#define STDC_CONTRACT_UNSPECIFIED          0x00
#define STDC_CONTRACT_PREDICATE_FALSE      0x01
#define STDC_CONTRACT_EVALUATION_EXCEPTION 0x02

// P3290 C API -- explicit location variants
__attribute__((__noreturn__))
void stdc_handle_enforced_contract_violation_explicit(
    const char* __comment, const char* __file,
    const char* __func, unsigned __line);

void stdc_handle_observed_contract_violation_explicit(
    const char* __comment, const char* __file,
    const char* __func, unsigned __line);

__attribute__((__noreturn__))
void stdc_handle_quick_enforced_contract_violation_explicit(
    const char* __comment, const char* __file,
    const char* __func, unsigned __line);

// P3290 C API -- convenience macros
#define stdc_handle_enforced_contract_violation(comment) \
    stdc_handle_enforced_contract_violation_explicit(     \
        (comment), __FILE__, __func__, __LINE__)

#define stdc_handle_observed_contract_violation(comment) \
    stdc_handle_observed_contract_violation_explicit(     \
        (comment), __FILE__, __func__, __LINE__)

#define stdc_handle_quick_enforced_contract_violation(comment) \
    stdc_handle_quick_enforced_contract_violation_explicit(     \
        (comment), __FILE__, __func__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif // _CONTRACTS_H
