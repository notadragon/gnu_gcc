// D4405's proof optimization level (DECISIONS.md E162-E168, E205):
// -fcontracts-prove-optimize= accepts only 0, 1, 2, 3 and follow (E168
// item 2: no s, z or g level).
// { dg-do compile }
// { dg-options "-fcontracts-prove-optimize=4 -fcontracts-prove-optimize=s" }
// { dg-error "the proof optimization level must be '0', '1', '2', '3' or 'follow', not '4'" "" { target *-*-* } 0 }
// { dg-error "the proof optimization level must be '0', '1', '2', '3' or 'follow', not 's'" "" { target *-*-* } 0 }
// { dg-message "valid arguments to '-fcontracts-prove-optimize=' are: 0 1 2 3 follow" "" { target *-*-* } 0 }

int i;
