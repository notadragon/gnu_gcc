| seq | commit | non-test +/- | test +/- | files | depends |
|-----|--------|--------------|----------|-------|---------|
| 120 | 0120-constexpr-vbase-lifetime | +0/-0 | +94/-0 | 1 | -- |
| 130 | 0130-explicit-inst-pack-cv | +0/-0 | +31/-0 | 1 | -- |
| 140 | 0140-constexpr-address-before-ctor | +0/-0 | +93/-0 | 1 | -- |
| 150 | 0150-this-in-xobj-declaration | +0/-0 | +52/-0 | 1 | -- |
| 160 | 0160-constexpr-heap-object-lifetime | +80/-0 | +161/-0 | 5 | -- |
| 170 | 0170-postcondition-undeduced-result-ice | +36/-1 | +131/-0 | 2 | -- |
| 180 | 0180-libstdcxx-contracts-cxxmin | +11/-6 | +0/-0 | 2 | -- |
| 190 | 0190-ubsan-null-ptrmem-object | +26/-0 | +15/-0 | 4 | -- |
| 200 | 0200-block-scope-first-decl-contract | +8/-0 | +43/-0 | 3 | -- |
| 210 | 0210-pch-contract-semantic | +5/-0 | +21/-0 | 3 | -- |
| 220 | 0220-ctor-pre-qualified-member | +29/-0 | +20/-0 | 2 | -- |
| 230 | 0230-module-template-contracts | +10/-8 | +45/-0 | 3 | -- |
| 240 | 0240-contract-explicit-copy-capture | +13/-0 | +35/-0 | 2 | -- |
| 250 | 0250-constexpr-unreported-incomplete-contract | +3/-1 | +44/-0 | 2 | -- |
| 260 | 0260-friend-contract-class-scope | +14/-6 | +30/-0 | 3 | -- |
| 270 | 0270-stmt-expr-local-lambda-contract | +52/-3 | +149/-0 | 6 | -- |
| 280 | 0280-generic-lambda-pre-capture | +14/-0 | +75/-0 | 2 | -- |
| 300 | 0300-typeid-polymorphic-capture | +0/-0 | +40/-0 | 1 | -- |
| 330 | 0330-contract-predicate-unexpanded-pack | +5/-0 | +31/-0 | 2 | -- |
| 340 | 0340-pch-contract-tables | +33/-5 | +189/-0 | 5 | -- |
| 360 | 0360-inherited-contract-test-tidying | +0/-0 | +65/-66 | 35 | -- |
| 370 | 0370-noexcept-array-filler | +30/-0 | +31/-0 | 4 | -- |
| 380 | 0380-noexcept-delete-null | +0/-0 | +26/-0 | 1 | -- |
| 1000 | 1000-p2900-fixes | +166/-53 | +0/-0 | 3 | -- |
| 1010 | 1010-constexpr-named-result | +37/-0 | +184/-0 | 3 | -- |
| 1020 | 1020-constexpr-side-effect | +164/-8 | +831/-0 | 7 | -- |
| 1025 | 1025-contract-discardable-evaluation | +44/-0 | +172/-0 | 5 | -- |
| 1030 | 1030-constexpr-repeat-call | +0/-0 | +441/-0 | 2 | 1020-constexpr-side-effect |
| 1040 | 1040-pack-reference-constify | +36/-1 | +110/-0 | 2 | -- |
| 1050 | 1050-predicate-lambda-constify | +115/-6 | +248/-0 | 9 | -- |
| 1060 | 1060-postcondition-odr-use | +363/-29 | +420/-0 | 13 | 1050-predicate-lambda-constify |
| 1070 | 1070-lambda-capture-constify | +136/-3 | +122/-0 | 6 | 1050-predicate-lambda-constify |
| 1080 | 1080-coroutine-postcondition-param | +52/-0 | +174/-0 | 5 | 1060-postcondition-odr-use |
| 1090 | 1090-contract-on-deleted-or-defaulted | +54/-1 | +97/-5 | 12 | -- |
| 1100 | 1100-postcondition-const-across-redeclarations | +210/-21 | +311/-0 | 6 | 1060-postcondition-odr-use |
| 1110 | 1110-postcondition-pack-resubstitution | +55/-0 | +197/-0 | 3 | -- |
| 1120 | 1120-outlined-checks-by-reference | +138/-19 | +103/-2 | 4 | -- |
| 1130 | 1130-result-binding-one-object | +148/-0 | +510/-5 | 6 | -- |
| 1140 | 1140-retval-destroyed-on-unwind | +198/-12 | +870/-0 | 9 | 1130-result-binding-one-object |
| 1160 | 1160-lambda-in-postcondition-result-name | +22/-1 | +139/-0 | 3 | -- |
| 1170 | 1170-result-name-introducer | +87/-16 | +124/-0 | 4 | -- |
| 1180 | 1180-lambda-this-capture-remap | +11/-0 | +148/-0 | 2 | -- |
| 1190 | 1190-lambda-capture-in-predicate | +45/-3 | +176/-0 | 3 | 1070-lambda-capture-constify |
| 1200 | 1200-xobj-member-in-predicate | +75/-5 | +293/-0 | 4 | -- |
| 1220 | 1220-noexcept-body-wrapper-detected | +18/-6 | +40/-0 | 2 | -- |
| 1230 | 1230-lambda-capture-bind-through-statement-list | +32/-8 | +119/-0 | 2 | -- |
| 1240 | 1240-contract-specifier-at-declarator-position | +681/-40 | +308/-2 | 10 | -- |
| 1250 | 1250-contract-on-non-function-declarator | +124/-0 | +354/-0 | 6 | 1240-contract-specifier-at-declarator-position |
| 1260 | 1260-outlined-lambda-capture-proxy | +47/-1 | +82/-0 | 5 | -- |
| 1270 | 1270-contract-only-capture-detection | +78/-7 | +122/-0 | 6 | -- |
| 1290 | 1290-lambda-in-declaration-contract | +28/-3 | +107/-0 | 9 | -- |
| 1300 | 1300-explicit-specialization-contracts | +122/-9 | +306/-113 | 10 | -- |
| 1310 | 1310-contract-comment-from-expression | +26/-0 | +33/-0 | 5 | -- |
| 1350 | 1350-contract-this-pointer-to-const | +93/-1 | +56/-0 | 7 | -- |
| 1360 | 1360-contract-only-capture-after-pruning | +101/-21 | +161/-0 | 6 | -- |
| 1370 | 1370-nested-copy-capture-in-predicate | +20/-0 | +41/-0 | 2 | -- |
| 1380 | 1380-await-in-predicate | +41/-0 | +42/-0 | 2 | -- |
| 1390 | 1390-pre-post-extension-spellings | +11/-0 | +93/-0 | 3 | -- |
| 1900 | 1900-p2900-expanded-test-coverage | +0/-0 | +3990/-25 | 96 | -- |
| 2000 | 2000-refactoring | +0/-11 | +0/-0 | 1 | -- |
| 2020 | 2020-defarg-cache-mode | +17/-4 | +0/-0 | 1 | -- |
| 2030 | 2030-diagnostic-message-parser | +73/-32 | +0/-0 | 1 | -- |
| 2200 | 2200-libcontracts | +19860/-557 | +353/-0 | 55 | -- |
| 2400 | 2400-p3595 | +3444/-126 | +3296/-2 | 192 | -- |
| 2600 | 2600-source-location-machinery | +113/-133 | +32/-0 | 3 | 2200-libcontracts |
| 2900 | 2900-umbrella-option | +17/-0 | +58/-0 | 6 | -- |
| 3000 | 3000-p3097 | +209/-21 | +4205/-0 | 83 | -- |
| 3300 | 3300-p3290 | +438/-2 | +892/-0 | 43 | -- |
| 3600 | 3600-p3099 | +161/-2 | +666/-0 | 31 | -- |
| 3610 | 3610-constexpr-contract-evaluation | +153/-35 | +9982/-0 | 17 | 1020-constexpr-side-effect, 2400-p3595, 3600-p3099 |
| 4000 | 4000-p3098 | +2137/-57 | +3274/-0 | 100 | -- |
| 4010 | 4010-p3097-p3098-combined | +0/-0 | +915/-0 | 5 | 3000-p3097, 4000-p3098 |
| 4020 | 4020-instantiate-on-use | +91/-0 | +146/-0 | 5 | 2400-p3595, 3000-p3097, 4000-p3098 |
| 4200 | 4200-p3400-core | +1679/-40 | +3480/-0 | 89 | -- |
| 4220 | 4220-p3400-facet-compute-semantic | +94/-0 | +285/-0 | 9 | 4200-p3400-core |
| 4230 | 4230-p3400-facet-allowed-semantics | +14/-0 | +586/-0 | 11 | 4200-p3400-core, 4220-p3400-facet-compute-semantic |
| 4240 | 4240-p3400-facet-identification | +168/-1 | +446/-0 | 8 | 4200-p3400-core |
| 4250 | 4250-p3400-facet-string-transforms | +115/-0 | +504/-0 | 7 | 4200-p3400-core |
| 4260 | 4260-p3400-facet-local-violation-handler | +98/-41 | +926/-0 | 13 | 4200-p3400-core |
| 4270 | 4270-p3400-bypass-rethrowing-local-handler | +718/-1 | +832/-0 | 15 | 4200-p3400-core, 4260-p3400-facet-local-violation-handler |
| 4280 | 4280-p3400-facet-queryable | +79/-4 | +603/-0 | 9 | 4200-p3400-core, 4260-p3400-facet-local-violation-handler |
| 4600 | 4600-p4283 | +163/-2 | +1135/-0 | 38 | -- |
| 4620 | 4620-contract-extension-fields | +215/-18 | +74/-0 | 4 | 1000-p2900-fixes, 3000-p3097, 3600-p3099, 4000-p3098, 4200-p3400-core, 4600-p4283 |
| 4630 | 4630-label-and-requires-parsing | +307/-0 | +0/-0 | 1 | 1240-contract-specifier-at-declarator-position, 4200-p3400-core, 4600-p4283 |
| 4640 | 4640-contract-substitution-plumbing | +455/-30 | +870/-0 | 11 | 1000-p2900-fixes, 4000-p3098, 4200-p3400-core, 4600-p4283 |
| 4650 | 4650-reentrancy-everything | +0/-0 | +126/-0 | 1 | 3000-p3097, 3600-p3099, 4000-p3098, 4200-p3400-core, 4600-p4283, 4620-contract-extension-fields, 4630-label-and-requires-parsing, 4640-contract-substitution-plumbing |
| 4800 | 4800-p4298 | +161/-1 | +820/-0 | 41 | -- |
| 4810 | 4810-libstdcxx-is-nonthrowing-test | +0/-0 | +20/-0 | 1 | 4200-p3400-core, 4800-p4298 |
| 5000 | 5000-p4299 | +1524/-13 | +2447/-0 | 102 | -- |
| 5200 | 5200-p4301 | +64/-0 | +214/-0 | 17 | -- |
| 5210 | 5210-libstdcxx-contracts-ftm-test | +0/-0 | +44/-0 | 1 | 3300-p3290, 3600-p3099, 4200-p3400-core, 5200-p4301 |
| 6000 | 6000-p3100-core | +1738/-19 | +1555/-1 | 124 | -- |
| 6030 | 6030-p3100-ubsan-runtime | +365/-1 | +269/-0 | 12 | 6000-p3100-core |
| 6040 | 6040-p3100-asan-runtime | +369/-13 | +1149/-0 | 25 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6050 | 6050-p3100-tsan-runtime | +138/-0 | +395/-0 | 11 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6100 | 6100-p3100-check-vptr | +1/-0 | +408/-0 | 11 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6110 | 6110-p3100-check-null-and-alignment | +369/-32 | +1264/-0 | 57 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6120 | 6120-p3100-check-object-size | +0/-0 | +80/-0 | 3 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6130 | 6130-p3100-check-nonnull-attribute | +0/-0 | +119/-0 | 4 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6140 | 6140-p3100-check-returns-nonnull-attribute | +0/-0 | +86/-0 | 3 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6150 | 6150-p3100-check-pointer-overflow | +1/-0 | +136/-0 | 6 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6170 | 6170-p3100-check-shift | +35/-0 | +223/-0 | 15 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6180 | 6180-p3100-check-integer-divide | +121/-6 | +619/-0 | 35 | 6000-p3100-core, 6030-p3100-ubsan-runtime, 6170-p3100-check-shift |
| 6190 | 6190-p3100-check-signed-integer-overflow | +153/-11 | +245/-0 | 18 | 6000-p3100-core, 6030-p3100-ubsan-runtime, 6110-p3100-check-null-and-alignment |
| 6200 | 6200-p3100-check-invalid-value | +181/-0 | +270/-0 | 14 | 6000-p3100-core, 6030-p3100-ubsan-runtime, 6110-p3100-check-null-and-alignment |
| 6210 | 6210-p3100-check-float-cast-overflow | +52/-6 | +279/-0 | 18 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6220 | 6220-p3100-check-bounds | +156/-6 | +258/-0 | 17 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6230 | 6230-p3100-check-flow-off | +232/-31 | +712/-0 | 36 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6240 | 6240-p3100-check-unreachable | +1/-0 | +29/-0 | 2 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6250 | 6250-p3100-check-vla-bound | +1/-0 | +29/-0 | 2 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6260 | 6260-p3100-check-builtin | +1/-0 | +29/-0 | 2 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6270 | 6270-p3100-check-float-divide-by-zero | +1/-0 | +140/-0 | 4 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6300 | 6300-p3100-check-pure-virtual | +119/-6 | +311/-0 | 16 | 6000-p3100-core |
| 6310 | 6310-p3100-check-enum-cast | +117/-2 | +269/-0 | 20 | 6000-p3100-core, 6210-p3100-check-float-cast-overflow |
| 6320 | 6320-p3100-check-coroutine-flow-off | +79/-0 | +176/-0 | 11 | 6000-p3100-core, 6230-p3100-check-flow-off |
| 6330 | 6330-p3100-check-assume-attribute | +109/-3 | +515/-0 | 26 | 6000-p3100-core |
| 6900 | 6900-cross-paper-options | +49/-5 | +0/-0 | 3 | 2900-umbrella-option, 1020-constexpr-side-effect, 2400-p3595, 3000-p3097, 3300-p3290, 3600-p3099, 4000-p3098, 4200-p3400-core, 4600-p4283, 4800-p4298, 5000-p4299, 5200-p4301, 6000-p3100-core |
| 6950 | 6950-libstdcxx-contracts-ftm-below-cxx26 | +0/-0 | +179/-0 | 3 | 0180-libstdcxx-contracts-cxxmin, 3300-p3290, 3600-p3099, 4200-p3400-core, 5200-p4301, 6000-p3100-core |
| 9900 | 9900-fork-metadata | +28713/-0 | +0/-0 | 324 | -- |
| 9999 | 9999-unknown | +0/-0 | +0/-0 | 0 | -- |
