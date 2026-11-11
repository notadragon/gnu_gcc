/* Of two -fmacro-prefix-map= options whose old prefixes both match, the
   last given wins.
   Mirror: edgcpp_compiler tests/tests/contracts/macro_prefix_map/
   gxx_options.sft.cpp.  */
/* { dg-do compile } */
/* { dg-options "-fmacro-prefix-map==FIRST/ -fmacro-prefix-map==SECOND/" } */

#pragma message "FILE is " __FILE__	/* { dg-message "FILE is SECOND/" } */
#line 100 "././site.exp"
#pragma message "LINE is " __FILE__	/* { dg-message "LINE is SECOND/\\./\\./site\\.exp'" "" { target *-*-* } 100 } */
