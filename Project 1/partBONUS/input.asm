MOVC,R0,#10
MOVC,R1,#4044
STORE,R1,R0,#10
STORE,R0,R0,#30
MOVC,R3,#2
MOVC,R2,#30
MOVC,R2,#1
MUL,R8,R2,R10
SUB,R3,R3,R2
BZ,#16
MUL,R6,R0,R1
LOAD,R5,R0,#10
JUMP,R5,#-20
HALT,
LOAD,R7,R0,#30
STORE,R7,R0,#40