| seq | commit | non-test +/- | test +/- | files | depends |
|-----|--------|--------------|----------|-------|---------|
| 120 | 0120-constexpr-vbase-lifetime | +0/-0 | +95/-0 | 1 | -- |
| 130 | 0130-explicit-inst-pack-cv | +0/-0 | +35/-0 | 1 | -- |
| 140 | 0140-constexpr-address-before-ctor | +0/-0 | +55/-0 | 1 | -- |
| 150 | 0150-this-in-xobj-declaration | +0/-0 | +52/-0 | 1 | -- |
| 1000 | 1000-p2900-fixes | +79/-14 | +0/-0 | 3 | -- |
| 1010 | 1010-constexpr-named-result | +21/-0 | +23/-0 | 2 | -- |
| 1020 | 1020-constexpr-side-effect | +120/-9 | +649/-0 | 8 | -- |
| 1025 | 1025-contract-discardable-evaluation | +72/-0 | +78/-0 | 2 | 1020-constexpr-side-effect |
| 1030 | 1030-constexpr-repeat-call | +24/-2 | +164/-0 | 2 | 1020-constexpr-side-effect |
| 1040 | 1040-pack-reference-constify | +36/-1 | +110/-0 | 2 | -- |
| 1050 | 1050-predicate-lambda-constify | +111/-6 | +191/-0 | 6 | -- |
| 1060 | 1060-postcondition-odr-use | +265/-27 | +317/-0 | 7 | 1050-predicate-lambda-constify |
| 1070 | 1070-lambda-capture-constify | +139/-3 | +102/-0 | 5 | 1050-predicate-lambda-constify |
| 1080 | 1080-coroutine-postcondition-param | +55/-0 | +161/-0 | 5 | 1060-postcondition-odr-use |
| 1090 | 1090-contract-on-deleted-or-defaulted | +54/-1 | +77/-5 | 11 | -- |
| 1100 | 1100-postcondition-const-across-redeclarations | +179/-21 | +286/-0 | 5 | 1060-postcondition-odr-use |
| 1110 | 1110-postcondition-pack-resubstitution | +55/-0 | +197/-0 | 3 | -- |
| 1120 | 1120-outlined-checks-by-reference | +138/-19 | +102/-2 | 4 | -- |
| 1130 | 1130-result-binding-one-object | +133/-0 | +512/-5 | 6 | -- |
| 1140 | 1140-retval-destroyed-on-unwind | +51/-0 | +556/-0 | 5 | 1130-result-binding-one-object |
| 1150 | 1150-capture-note-location | +24/-1 | +69/-0 | 2 | -- |
| 1160 | 1160-lambda-in-postcondition-result-name | +19/-1 | +138/-0 | 3 | -- |
| 1170 | 1170-result-name-introducer | +89/-16 | +115/-0 | 3 | -- |
| 1180 | 1180-lambda-this-capture-remap | +11/-0 | +148/-0 | 2 | -- |
| 1190 | 1190-lambda-capture-in-predicate | +45/-3 | +175/-0 | 3 | 1070-lambda-capture-constify |
| 1200 | 1200-xobj-member-in-predicate | +12/-1 | +268/-0 | 3 | -- |
| 1210 | 1210-deferred-friend-redeclaration-limit | +12/-4 | +31/-0 | 2 | -- |
| 1220 | 1220-noexcept-body-wrapper-detected | +18/-6 | +40/-0 | 2 | -- |
| 1230 | 1230-lambda-capture-bind-through-statement-list | +32/-8 | +119/-0 | 2 | -- |
| 1240 | 1240-retval-cleanup-not-respliced | +13/-0 | +167/-0 | 2 | -- |
| 1900 | 1900-p2900-expanded-test-coverage | +0/-0 | +1617/-18 | 20 | -- |
| 2000 | 2000-refactoring | +3/-12 | +0/-0 | 2 | -- |
| 2020 | 2020-defarg-cache-mode | +14/-4 | +0/-0 | 1 | -- |
| 2030 | 2030-diagnostic-message-parser | +65/-30 | +0/-0 | 1 | -- |
| 2200 | 2200-libcontracts | +19919/-555 | +53/-0 | 39 | -- |
| 2400 | 2400-p3595 | +2822/-115 | +2265/-2 | 128 | -- |
| 2600 | 2600-source-location-machinery | +103/-124 | +32/-0 | 3 | 2200-libcontracts |
| 2900 | 2900-umbrella-option | +8/-0 | +45/-0 | 4 | -- |
| 3000 | 3000-p3097 | +187/-24 | +3392/-0 | 66 | -- |
| 3300 | 3300-p3290 | +407/-2 | +543/-0 | 37 | -- |
| 3600 | 3600-p3099 | +132/-2 | +434/-0 | 24 | -- |
| 3610 | 3610-constexpr-contract-evaluation | +127/-34 | +0/-0 | 1 | 1020-constexpr-side-effect, 2400-p3595, 3600-p3099 |
| 4000 | 4000-p3098 | +1583/-55 | +1670/-0 | 51 | -- |
| 4010 | 4010-p3097-p3098-combined | +0/-0 | +810/-0 | 4 | 3000-p3097, 4000-p3098 |
| 4020 | 4020-instantiate-on-use | +84/-0 | +146/-0 | 5 | 2400-p3595, 3000-p3097, 4000-p3098 |
| 4200 | 4200-p3400-core | +1509/-38 | +2663/-0 | 57 | -- |
| 4220 | 4220-p3400-facet-compute-semantic | +74/-0 | +101/-0 | 3 | 4200-p3400-core |
| 4230 | 4230-p3400-facet-allowed-semantics | +11/-0 | +468/-0 | 7 | 4200-p3400-core, 4220-p3400-facet-compute-semantic |
| 4240 | 4240-p3400-facet-identification | +90/-1 | +407/-0 | 7 | 4200-p3400-core |
| 4250 | 4250-p3400-facet-string-transforms | +93/-0 | +305/-0 | 4 | 4200-p3400-core |
| 4260 | 4260-p3400-facet-local-violation-handler | +88/-41 | +740/-0 | 10 | 4200-p3400-core |
| 4270 | 4270-p3400-bypass-rethrowing-local-handler | +677/-0 | +793/-0 | 12 | 4200-p3400-core, 4260-p3400-facet-local-violation-handler |
| 4280 | 4280-p3400-facet-queryable | +79/-4 | +578/-0 | 9 | 4200-p3400-core, 4260-p3400-facet-local-violation-handler |
| 4600 | 4600-p4283 | +97/-2 | +698/-0 | 27 | -- |
| 4620 | 4620-contract-extension-fields | +235/-19 | +76/-0 | 4 | 1000-p2900-fixes, 3000-p3097, 3600-p3099, 4000-p3098, 4200-p3400-core, 4600-p4283 |
| 4630 | 4630-label-and-requires-parsing | +152/-0 | +0/-0 | 1 | 4200-p3400-core, 4600-p4283 |
| 4640 | 4640-contract-substitution-plumbing | +331/-26 | +649/-0 | 4 | 1000-p2900-fixes, 4000-p3098, 4200-p3400-core, 4600-p4283 |
| 4650 | 4650-reentrancy-everything | +0/-0 | +126/-0 | 1 | 3000-p3097, 3600-p3099, 4000-p3098, 4200-p3400-core, 4600-p4283, 4620-contract-extension-fields, 4630-label-and-requires-parsing, 4640-contract-substitution-plumbing |
| 4800 | 4800-p4298 | +126/-1 | +527/-0 | 29 | -- |
| 4810 | 4810-libstdcxx-is-nonthrowing-test | +0/-0 | +20/-0 | 1 | 4200-p3400-core, 4800-p4298 |
| 5000 | 5000-p4299 | +830/-6 | +1373/-0 | 51 | -- |
| 5200 | 5200-p4301 | +60/-0 | +198/-0 | 15 | -- |
| 5210 | 5210-libstdcxx-contracts-ftm-test | +0/-0 | +31/-0 | 1 | 3300-p3290, 3600-p3099, 4200-p3400-core, 5200-p4301 |
| 6000 | 6000-p3100-core | +1556/-17 | +981/-0 | 94 | -- |
| 6030 | 6030-p3100-ubsan-runtime | +298/-1 | +0/-0 | 2 | 6000-p3100-core |
| 6040 | 6040-p3100-asan-runtime | +348/-13 | +904/-0 | 19 | 6000-p3100-core |
| 6050 | 6050-p3100-tsan-runtime | +105/-0 | +258/-0 | 6 | 6000-p3100-core |
| 6100 | 6100-p3100-check-vptr | +1/-0 | +400/-0 | 11 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6110 | 6110-p3100-check-null-and-alignment | +377/-34 | +1089/-0 | 53 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6120 | 6120-p3100-check-object-size | +0/-0 | +80/-0 | 3 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6130 | 6130-p3100-check-nonnull-attribute | +0/-0 | +119/-0 | 4 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6140 | 6140-p3100-check-returns-nonnull-attribute | +0/-0 | +86/-0 | 3 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6150 | 6150-p3100-check-pointer-overflow | +1/-0 | +136/-0 | 6 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6170 | 6170-p3100-check-shift | +39/-0 | +208/-0 | 15 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6180 | 6180-p3100-check-integer-divide | +132/-4 | +498/-0 | 31 | 6000-p3100-core, 6030-p3100-ubsan-runtime, 6170-p3100-check-shift |
| 6190 | 6190-p3100-check-signed-integer-overflow | +168/-7 | +181/-0 | 12 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6200 | 6200-p3100-check-invalid-value | +201/-0 | +261/-0 | 14 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6210 | 6210-p3100-check-float-cast-overflow | +58/-6 | +264/-0 | 18 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6220 | 6220-p3100-check-bounds | +78/-6 | +175/-0 | 15 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6230 | 6230-p3100-check-flow-off | +244/-31 | +661/-0 | 36 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6240 | 6240-p3100-check-unreachable | +1/-0 | +29/-0 | 2 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6250 | 6250-p3100-check-vla-bound | +1/-0 | +29/-0 | 2 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6260 | 6260-p3100-check-builtin | +1/-0 | +29/-0 | 2 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6270 | 6270-p3100-check-float-divide-by-zero | +1/-0 | +138/-0 | 4 | 6000-p3100-core, 6030-p3100-ubsan-runtime |
| 6300 | 6300-p3100-check-pure-virtual | +117/-6 | +309/-0 | 16 | 6000-p3100-core |
| 6310 | 6310-p3100-check-enum-cast | +108/-0 | +176/-0 | 16 | 6000-p3100-core, 6210-p3100-check-float-cast-overflow |
| 6320 | 6320-p3100-check-coroutine-flow-off | +97/-0 | +167/-0 | 11 | 6000-p3100-core, 6230-p3100-check-flow-off |
| 6330 | 6330-p3100-check-assume-attribute | +131/-3 | +464/-0 | 25 | 6000-p3100-core |
| 6900 | 6900-cross-paper-options | +31/-5 | +0/-0 | 3 | 2900-umbrella-option, 1020-constexpr-side-effect, 2400-p3595, 3000-p3097, 3300-p3290, 3600-p3099, 4000-p3098, 4200-p3400-core, 4600-p4283, 4800-p4298, 5000-p4299, 5200-p4301, 6000-p3100-core |
| 9900 | 9900-fork-metadata | +19207/-0 | +0/-0 | 179 | -- |
| 9999 | 9999-unknown | +0/-0 | +0/-0 | 0 | -- |
