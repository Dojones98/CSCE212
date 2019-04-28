#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

/* function opcode */
#define ADD 0
#define SUB 1
#define LWR 2
#define ADDI 5
#define LW  8
#define SW  9
#define BEQ 12
#define J   15

/**
 * handy for print the function string
 * @param Func
 * @return
 */
char * funcName(int Func) {
    switch (Func) {
        case ADD:
            return "ADD";
        case SUB:
            return "SUB";
        case LWR:
            return "LWR";
        case ADDI:
            return "ADDI";
        case LW:
            return "LW";
        case SW:
            return "SW";
        case BEQ:
            return "BEQ";
        case J:
            return "J";
    }
    return "";
}

union InstructionWord {
/* each has to be exactly 32-bit in total */
    struct RType {
        unsigned int unused:11;
        unsigned int Rd:5;
        unsigned int Rt:5;
        unsigned int Rs:5;
        unsigned int func:6;
    }rType;

    struct IType {
        int Imm:16;
        unsigned int Rt:5;
        unsigned int Rs:5;
        unsigned int func:6;
    }iType;

    struct JType {
        int Imm:26;
        unsigned int func:6;
    }jType;
};

/**
 * All the data path of the CPU
 */
struct datapath_t {
    //data path for the IF stage
    unsigned int PC;                 // Program counter
    int JTImm;                       // Jump target
    unsigned int PCplus4;            // PC+4
    int BTaddr;                      // branch target address
    int PCplus4OrBTaddr;             // For selecting between PC+4 or branch target address
    int PCnext;                      // The next PC after all are resolved

    //datapath for the ID/RF stage
    unsigned int Func:6;             // Func field of an instruction
    unsigned int RSselect:5;         // RS register select, for both R and I type instructions
    unsigned int RTselect:5;         // RT register select, for both R and I type instructions
    unsigned int RDselect:5;         // RD register select, only for AL instruction (add and sub)
    unsigned int RWselect:5;         // RW register select, from either RDselect (add and sub) or RTselect (LW and ADDI)
    int Imm;                         // The immediate field of an instruction, only for I-type instruction,
                                     // No bitfield 16 so sign-extended is automatically applied when this field is assigned
    unsigned int RSvalue;            // The value of RSselect register, fed to ALUin1
    unsigned int RTvalue;            // The value of RTselect register, fed to either ALUin2 for ADD, SUB, and BEQ
                                     // or to memory as data to be written to for SW

    //datapath for the EXE stage
    // no need ALUin1 since it is the same as RSvalue
    unsigned int ALUin2;             // The ALU input 2, selected from either RTvalue (ADD or SUB) or Imm (LW, SW and ADDI)
    unsigned int ALUout;             // ALU output, used for both R and I type instructions.

    //datapath for MEM stage
    unsigned int MEMout;             // Only for LW
    //datapath for the Address is the same as ALUout

    //datapath for WB stage
    unsigned int RWvalue;            // For writing data back to the register for ADD, SUB, ADDI, and LW
} datapath;

/**
 * All the control signal of the CPU
 */
struct control_t {
    unsigned int RegDst:1;
    unsigned int Jump:1;
    unsigned int Branch:1;
    unsigned int MemRead:1;
    unsigned int MemtoReg:1;
    unsigned int ALUOp:6;
    unsigned int MemWrite:1;
    unsigned int ALUSrc:1;
    unsigned int RegWrite:1;
    unsigned int Zero:1;
} control;

/* The major CPU components, mainly the IM, DM, PC, and registers. mux is implemented as a simple c function*/
char *InstructionMemory;
char *DataMemory;
int* RegisterFile;
int PC; /* program counter register */
int IR; /* instruction register */

/**
 * mux
 */
int mux(int input0, int input1, char select) {
    if (select == 0) return input0;
    else return input1;
}

//The trace file
FILE *cpusimTraceFile;

/**
 * fetch instruction word from instruction memory and update PC+4
 */
void fetch() {
    datapath.PC = PC;
    IR = *((int*)(&InstructionMemory[PC]));
    fprintf(cpusimTraceFile, "Fetch instruction %08x at PC %d\n", IR, PC);
    datapath.PCplus4 = datapath.PC + 4; /* we use + to simulate the adder for adding PC and 4 */
}

/**
 * decode the instruction word. This simulates the way hardware implements a decoder, which decomposes
 * the instruction word into pieces of all the possible types of instructions.
 */
void decode()  {
    union InstructionWord instrWord = *(union InstructionWord *) &IR;

    /* setting datapath: Func, RSselect, RTselect, RDselect, Imm and JTImm */
    datapath.Func = instrWord.iType.func;
    datapath.RSselect = instrWord.rType.Rs;
    datapath.RTselect = instrWord.rType.Rt;
    datapath.RDselect = instrWord.rType.Rd;
    datapath.Imm = instrWord.iType.Imm; /* automatically do sign extension */
    datapath.JTImm = instrWord.jType.Imm;
    fprintf(cpusimTraceFile, "\tDecode instruction (fun rs rt rd Imm JTImm): %s %d %d %d %d %d\n",
           funcName(datapath.Func), datapath.RSselect, datapath.RTselect, datapath.RDselect, datapath.Imm, datapath.JTImm);
}

/*
 * 1. Set each of the control signal according to the func code of the instruction
 * 2. Select RWselect based on the RegDst control
 * 3. Fetch data from register and put them on the data path, only for RS and RT register
 * 4. Select ALUin2 based on the control.ALUSrc
 * 5. Update the jump target (shift left by 2)
 */
void controlAndRegisterFetch() {
    switch (datapath.Func) {
        case ADD: {
            control.RegDst = 1;         // Select Rd as destination register
            control.Jump = 0;           // Not a jump instruction
            control.Branch = 0;         // Not a branch, to the AND
            control.MemRead = 0;        // Not memory read, to data memory
            control.MemtoReg = 0;       // Not for LW data to memory, for Mux 5
            control.ALUOp = datapath.Func;        // the ALU operation needs to perform
            control.MemWrite = 0;       // Not memory write (SW)
            control.ALUSrc = 0;         // for selecting RTvalue in Mux 4 (instead of Imm)
            control.RegWrite = 1;       // A signal to register to indicate that the instruction
            // needs to write the result to the register
            break;
        }
        case SUB: {
            // TODO: setting control for SUB
            break;
        }
        case LW: {
            // TODO: setting control for LW
            break;
        }
        case LWR: {
            // TODO: setting control for LWR
            break;
        }
        case SW: {
            // TODO: setting control for SW
            break;
        }
        case BEQ: {
            // TODO: setting control for BEQ
            break;
        }
        case ADDI: {
            // TODO: setting control for ADDI
            break;
        }
        case J: {
            // TODO: setting control for J
            break;
        }
    }

    /* TODO: setting datapath: RWselect, RSvalue, RTvalue, ALUin2 and JTImm */


    //write trace to file
    fprintf(cpusimTraceFile, "\tFetch register: Rs: Reg[%d]=%d, Rt: Reg[%d]=%d\n",
           datapath.RSselect, datapath.RSvalue, datapath.RTselect, datapath.RTvalue);
}

/**
 * 1. Select ALUin2 based on ALUSrc control
 * 2. Perform ALU operation on the input and ALUop, and update ALUout datapath
 * 3. Update the Zero control signal based on the output of ALU
 * 4. Update the BTaddr datapath
 */
void EXE() {
    //TODO: setting datapath: ALUin2, ALUout by doing either ADD or SUB depending on the ALUop control, and BTaddr
    //TODO: seeting control: Zero control depending on ALUout

    fprintf(cpusimTraceFile, "\tEXE: Ops %s, ALUout: %d, Zero: %d, BTaddr: %d\n",
           funcName(control.ALUOp), datapath.ALUout, control.Zero, datapath.BTaddr);
}

#define ReadDataMemoryWord(addr)     *((int*)(&DataMemory[addr]))
#define WriteDataMemoryWord(addr, word) *(int*)(&DataMemory[addr])=word
/**
 * 1. Read or write memory depending on the MemRead and MemWrite control
 * 2. Resolve branch or jump and calculate PCnext.
 */
void MEM() {
    if (control.MemRead) {
        //TODO: setting datapath: Memout if MemRead control is set. You should use ReadDataMemoryWord macro defined in front of this function

        fprintf(cpusimTraceFile, "\tMEM: LW from %d, value: %d\n", datapath.ALUout, datapath.MEMout);
    } // for LW|LWR instruction
    if (control.MemWrite) {
        //TODO: perform memory write using the WriteDataMemoryWord macro defined in front of this function

        fprintf(cpusimTraceFile, "\tMEM: SW at %d, value: %d\n", datapath.ALUout, datapath.RTvalue);
    } // for SW

    //TODO: setting datapath: PCplus4OrBTaddr and PCnext

    fprintf(cpusimTraceFile, "\tMEM: PCnext: %d\n", datapath.PCnext);
}

/**
 * 1. Select RWvalue
 * 2. Write to register file if RegWrite control is set
 */
void WB() {
    //TODO: setting datapath: RWvalue

    if (control.RegWrite) {
        //TODO: Write to register file

        fprintf(cpusimTraceFile, "\tWB: Reg[%d] = %d\n", datapath.RWselect, datapath.RWvalue);
    }
}

int main(int argc, char *argv[]) {
    /* fileName should be provided as the first parameter of the program */
    if (argc != 2) {
        printf("Usage: cpusim <fileName>\n");
        return 1;
    }

    /* initialize the CPU components, mainly the IM, DM, PC, registers, etc */
    InstructionMemory = (char*) malloc(1024*1024); /* 1Kbyes */
    DataMemory = (char*) malloc(1024*1024*1024); /* 1Mbyes */
    RegisterFile = (int*) malloc(32*4); /* 32 32-bit registers */
    RegisterFile[0] = 0; //$s0 is 0

    // Load the binary file into instruction memory
    FILE *binFile = fopen(argv[1], "r");
    if (binFile == NULL){
        printf("Could not open file %s",argv[1]);
        return 1;
    }
    int numInstr = 0;
    while (fscanf(binFile, "%08x\n", (unsigned int *)&InstructionMemory[numInstr*4]) != EOF) {
        //printf("%08x\n", (unsigned int *)&InstructionMemory[numInstr*4]);
        numInstr++;
    }
    fclose(binFile);

    /*
     * open the trace file to collect traces
     */
    cpusimTraceFile = fopen("cpusim_trace.txt", "w");

    // the program starts from the first instruction
    int programEntry = 0;
    datapath.PC=programEntry;

    /* init memory and register for test.asm program
     * A and B each array has 256 int elements. We only need to init B.
     * The base addresses of A and B are stored in register $s1 and $s2
     */
    int N = 16;
    RegisterFile[1] = 0;    /* memory address for A, A is in DataMemory starting from 0 for N*4 bytes */
    RegisterFile[2] = N*4;  /* memory address for B, B is in DataMemory starting from N*4 for N*4 bytes
                             * B can start from any address within the range as long as it does not overlap with A.
                             */
    //manually initialize B
    int i;
    int * A = (int*)(&DataMemory[RegisterFile[1]]);
    int * B = (int*)(&DataMemory[RegisterFile[2]]);
    srand(time(NULL));
    for (i=0; i<N; i++) B[i] = rand();

    /* CPU simulation loop, each iteration executes one instruction */
    int IC = 0;
    for(;;) {
        fetch();
        decode();
        controlAndRegisterFetch();
        EXE();
        MEM();
        WB();

        PC = datapath.PCnext;
        IC++;
        if (PC >= 9999) break; // J <very far address> is just the easiest way to terminate the program
        if (PC == 0) {//* goes to infinite loop for the test.asm program, we terminate */
            fprintf(cpusimTraceFile, "Simulation goes to infinits loop of test.asm program, terminate it\n");
            break;
        }
    }

    /* verification of the simulation with our own computation of test.asm */
    int VA[N];
    int success = 1;
    for (i=1; i != N-2; i++) {
        VA[i] = B[i - 1] + B[i] + B[i + 1];
        if (A[i] != VA[i]) {
            fprintf(cpusimTraceFile, "Verification failed: VA[%d]: %d, Sim Number: %d\n", i, VA[i], A[i]);
            success = 0;
        }
    }
    if (success) {
        printf("Simulation and Verification Passed Successfully!\n");
        fprintf(cpusimTraceFile, "===================================================\n");
        fprintf(cpusimTraceFile, "Simulation and Verification Passed Successfully!\n");
        fprintf(cpusimTraceFile, "Simulation Summary: \n");
        fprintf(cpusimTraceFile, "\t Num of Instruction Executed: %d\n", IC);
    } else {
        printf("Verification Failed!\n");
    }
    fclose(cpusimTraceFile);

    return 0;
}
