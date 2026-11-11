/* A -ffile-prefix-map= is tried before any -fmacro-prefix-map= for
   __FILE__, whatever their order: here the macro map comes first.
   Mirror: edgcpp_compiler tests/tests/contracts/macro_prefix_map/
   gxx_options.sft.cpp.  */
/* { dg-do compile } */
/* { dg-options "-fmacro-prefix-map==MACRO/ -ffile-prefix-map==FILE/" } */

#pragma message "FILE is " __FILE__	/* { dg-message "FILE is FILE/" } */
#line 100 "././site.exp"
#pragma message "LINE is " __FILE__	/* { dg-message "LINE is FILE/\\./\\./site\\.exp'" "" { target *-*-* } 100 } */
