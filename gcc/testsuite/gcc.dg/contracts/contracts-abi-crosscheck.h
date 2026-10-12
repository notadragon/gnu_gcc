/* The contracts ABI is spelled twice in this tree: libcontracts' C header
   (libcontracts/contracts-abi.h) and libstdc++'s <bits/contracts_abi.h>.
   Nothing shares the two, so this table is checked against each -- as C with
   the first (gcc.dg/contracts/c-contracts-abi-header-crosscheck.c), as C++
   with the second (g++.dg/contracts/cpp26/contracts-abi-header-crosscheck.C)
   -- and both must satisfy it: every CXA_* value and the size, alignment and
   member offsets of every wire-format type.  The includer brings in its
   header first.

   Mirror: clang/test/Contracts/Runnable/contracts-abi-header-crosscheck.cpp
   in the llvm_llvm-project fork, which compiles one file against each of its
   two headers and diffs what the two programs print.  */

#include <stddef.h>

#ifdef __cplusplus
using namespace __cxxabiv1;
# define CXA_CHECK(e) static_assert (e, #e)
# define CXA_ALIGNOF(t) alignof (t)
#else
# define CXA_CHECK(e) _Static_assert (e, #e)
# define CXA_ALIGNOF(t) _Alignof (t)
#endif

#define CXA_P sizeof (void *)
#define CXA_A CXA_ALIGNOF (void *)

CXA_CHECK (CXA_AK_UNSPECIFIED == 0x00);
CXA_CHECK (CXA_AK_PRE == 0x01);
CXA_CHECK (CXA_AK_POST == 0x02);
CXA_CHECK (CXA_AK_ASSERT == 0x03);
CXA_CHECK (CXA_AK_MANUAL == 0x04);
CXA_CHECK (CXA_AK_CASSERT == 0x05);
CXA_CHECK (CXA_AK_POST_CAPTURE == 0x06);
CXA_CHECK (CXA_AK_IMPLICIT == 0x07);

CXA_CHECK (CXA_ES_UNSPECIFIED == 0x00);
CXA_CHECK (CXA_ES_IGNORE == 0x01);
CXA_CHECK (CXA_ES_OBSERVE == 0x02);
CXA_CHECK (CXA_ES_ENFORCE == 0x03);
CXA_CHECK (CXA_ES_QUICK_ENFORCE == 0x04);
CXA_CHECK (CXA_ES_NOEXCEPT_OBSERVE == 0x06);
CXA_CHECK (CXA_ES_NOEXCEPT_ENFORCE == 0x07);

CXA_CHECK (CXA_DM_UNSPECIFIED == 0x00);
CXA_CHECK (CXA_DM_PREDICATE_FALSE == 0x01);
CXA_CHECK (CXA_DM_EVALUATION_EXCEPTION == 0x02);

CXA_CHECK (CXA_FIELD_SOURCE_LOCATION == 0x01);
CXA_CHECK (CXA_FIELD_COMMENT == 0x02);
CXA_CHECK (CXA_FIELD_MESSAGE == 0x03);
CXA_CHECK (CXA_FIELD_LOCAL_HANDLER == 0x04);
CXA_CHECK (CXA_FIELD_QUERY_FUNCTION == 0x05);
CXA_CHECK (CXA_FIELD_LABEL_PTR == 0x06);
CXA_CHECK (CXA_FIELD_ASSERTION_KIND == 0x07);
CXA_CHECK (CXA_FIELD_EVALUATION_SEMANTIC == 0x08);
CXA_CHECK (CXA_FIELD_DETECTION_MODE == 0x09);
CXA_CHECK (CXA_FIELD_EXCEPTION_PTR == 0x0A);
CXA_CHECK (CXA_FIELD_REPORT == 0x0B);
CXA_CHECK (CXA_FIELD_EXTENDED == 0x40);

CXA_CHECK (CXA_VENDOR_GENERIC == 0x0);
CXA_CHECK (CXA_VENDOR_GCC == 0x1);
CXA_CHECK (CXA_VENDOR_CLANG == 0x2);
CXA_CHECK (CXA_VENDOR_MSVC == 0x3);

/* Two pointers, then two unsigned ints.  */
CXA_CHECK (CXA_ALIGNOF (struct __cxa_source_location) == CXA_A);
CXA_CHECK (sizeof (struct __cxa_source_location)
	   == (2 * CXA_P + 2 * sizeof (unsigned) + CXA_A - 1) / CXA_A * CXA_A);
CXA_CHECK (offsetof (struct __cxa_source_location, file_name) == 0);
CXA_CHECK (offsetof (struct __cxa_source_location, function_name) == CXA_P);
CXA_CHECK (offsetof (struct __cxa_source_location, line) == 2 * CXA_P);
CXA_CHECK (offsetof (struct __cxa_source_location, column)
	   == 2 * CXA_P + sizeof (unsigned));
CXA_CHECK (sizeof (unsigned) == 4);

/* A byte offset or a pointer, both pointer-sized.  */
CXA_CHECK (sizeof (union __cxa_descriptor_data_t) == CXA_P);
CXA_CHECK (CXA_ALIGNOF (union __cxa_descriptor_data_t) == CXA_A);
CXA_CHECK (offsetof (union __cxa_descriptor_data_t, offset) == 0);
CXA_CHECK (offsetof (union __cxa_descriptor_data_t, pointer) == 0);
CXA_CHECK (sizeof (((union __cxa_descriptor_data_t *) 0)->offset) == CXA_P);

/* Header byte, entry count, then the field IDs; data[] is found by
   alignment, which the P3290 cross-check (p3290-data-block-crosscheck.C)
   exercises through both libraries' lookups.  */
CXA_CHECK (sizeof (struct __cxa_descriptor_table_t) == 2);
CXA_CHECK (CXA_ALIGNOF (struct __cxa_descriptor_table_t) == 1);
CXA_CHECK (offsetof (struct __cxa_descriptor_table_t, header) == 0);
CXA_CHECK (offsetof (struct __cxa_descriptor_table_t, num_entries) == 1);
CXA_CHECK (offsetof (struct __cxa_descriptor_table_t, field_ids) == 2);

CXA_CHECK (sizeof (struct __cxa_contract_data_block) == 2 * CXA_P);
CXA_CHECK (CXA_ALIGNOF (struct __cxa_contract_data_block) == CXA_A);
CXA_CHECK (offsetof (struct __cxa_contract_data_block, descriptor) == 0);
CXA_CHECK (offsetof (struct __cxa_contract_data_block, next) == CXA_P);

CXA_CHECK (sizeof (struct __cxa_contract_report_populator) == 2 * CXA_P);
CXA_CHECK (CXA_ALIGNOF (struct __cxa_contract_report_populator) == CXA_A);
CXA_CHECK (offsetof (struct __cxa_contract_report_populator, populate) == 0);
CXA_CHECK (offsetof (struct __cxa_contract_report_populator, ctx) == CXA_P);
