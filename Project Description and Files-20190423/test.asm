# This program implements the following 1-D convolution kind of algorithm
# int i, N;
# int A[N], B[N];
# for (i=1; i != N-2; i++)
#    A[i] = B[i-1] + B[i] + B[i+1];
# Assume the address of A and B are stored in register $s1 and $s2.
# We use $s3 and $s4 for i and N variables
# It is very important that branch or jump target need to explicitly set as numbers
# branch target is the number of instructions between the branch instruction and the
# target instruction (not includig the branch and target instruction itself).
# Jump target is the absolution instruction number. If the transfer because of
# branch or jump is to go backward, target should be negative.

ADDI, $s3, $s0, 1            # instruction #0, i = 1;
ADDI, $s4, $s0, 16           # instruction #1, Init N=16
ADDI, $s4, $s4, -2           # instruction #2, $s4 has 24
# loop label:   which is instruction 3
BEQ, $s3, $s4, 12            # 3, Jump to the end of the code, use relative address 12
                             # since there is 12 instructions in between this and last instruction
ADD, $s11, $s3, $s3          # 4,  i = i*2
ADD, $s11, $s11, $s11        # 5,  i = i*2*2, now $s11 has i*4
ADD, $s5, $s11, $s2          # 6,  &B[i] is now in $s5
LW,  $s6, $s5, -4            # 7,  B[i-1] is now in $s6
LW,  $s7, $s5, 0             # 8,  B[i] is now in $s7
LW,  $s8, $s5, 4             # 9,  B[i+1] is now in $s8
ADD, $s9, $s6, $s7           # 10, B[i-1] + B[i]
ADD, $s9, $s8, $s9           # 11, B[i-1] + B[i] + B[i+1]
ADD, $s10, $s11, $s1         # 12, &A[i] is now in $s10
SW,  $s9, $s10, 0            # 13, A[i] stored the result
ADDI, $s3, $s3, 1            # 14, i++
J, 3                         # 15, Jump instruction #3 (BEQ instruction, use absolute address)
# exit label:                # 16,
J, 999999                    # Implemented in simulator to indicate to terminate the program.

