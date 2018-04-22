# COMP 506: Compiler Construction

This repository includes a optimizer for "ILOC" language.

## What is ILOC

ILOC is an intermidiate representation for higher level language. The ILOC instruction set is taken from the book, Engineering A Compiler, published by the Morgan-Kaufmann imprint of Elsevier.

The ILOC operations supported in the simulator fall into four basic categories: computa-tional operations, data movement operations, control-flow operations, and output operations.

An ILOC instruction is either a single operation, or a group of operations enclosed insquare brackets and separted by semicolons, as in `[ op1 ; op2 ]`. An instruction label in ILOCconsists of an alphabetic character followed by zero or more alphanumeric characters. Any ILOC instruction may be labeled; the label precedes the instruction and is followed by acolon, as in `L01: add r1, r2 ⇒ r3`, or `L02: [ add r1, r2 ⇒ r3; i2i r0 ⇒ r4 ]`.

ILOC is case sensitive. The only uppercase letters in ILOC are the 'A', 'I', and 'O' used to specify address modes and immediate operations. In the tables, $r_i$ represents a register name; the subscripts make explicit the correspondence between operands in the “Format” column and the "Meaning" column. The notation $c_i$ represents an integer constant, and Li represents a label. "WORD[*ex*]" indicates the con-tents of the word of data memory at the location specified by ex. The address expression, ex,must be word-aligned—that is (ex MOD 4) must be 0. "BYTE[*ex*]" indicates the contents ofthe byte of data memory at the location specified by *ex*, without an alignment constraint on *ex*.

Register names have an initial **r** followed immediately by a non-negative integer. The '**r**' is case sensitive (as is all of ILOC). Leading zeroes in the register name are not significant;thus `r017` and `r17` refer to the same register. Arguments that do not begin with an '**r**' which appear as a c in the tables, are assumed to be positive integers constants in the range0 to $2^{31}-1$.

Blanks and tabs are treated as whitespace. All ILOC opcodes must be followed by whitespace—any combination of blanks or tabs. Whitespace preceding and following other symbols is optional. Whitespace may not appear within operation names, register names, or the assignment symbol. A double slash (“//”) indicates that the rest of the line is a comment. Empty lines may appear in the input; the simulator will ignore them.

## Optimizer

Two code optimization algorithms are implemented: ***value numbering*** and ***loop unrolling***.

1. ***value numbering***: local value numbering or superlocal value numbering; specified with a -v flag

2. ***loop unrolling***: unroll inner loops by a factor of four; specified with a -u flag

3. ***loop-invariant code motion*** (*not implemented*): find computations that are invariant in inner loops andmove them to a place where they execute less often; specified with a -i flag

## Usage

The optimizer accepts a command line of the form:

    `opt [flags] file.i`

where flags specify the optimizations to run (and the order in which to runt them, and `file.i` is the name of the ILOC input file that will be optimized. The optimizer write its output file to `stdout`.

The order in which the optimization switches appear determines the order in which the optimizations apply. For example,

    `opt -v -i file.i`

optimizes the file `file.i`, applying value numbering followed by loop-invariant code motion, while the command line:

    `opt -i -v file.i`

would apply loop-invariant code motion before value numbering.
