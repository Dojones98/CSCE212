#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* helper function for dealing with string with leading and trailing spaces
 * and for removing $s from a register operand
 */
char *removeWhite(char *str);
char * remove$s(char * operand);

/* function opcode */
#define ADD 0
#define SUB 1
#define LWR 2
#define ADDI 5
#define LW  8
#define SW  9
#define BEQ 12
#define J   15

/* each has to be exactly 32-bit in total */
struct AnyInstruction {
    unsigned int otherField:26;
    unsigned int func:6;
};

/* The definition of an instruction word, the order of these fields are important, which
 * should be listed from the least-significant. The struct definition leverage c programming
 * bitwidth specification for struct field, which make the coding much easier*/

/**
 * for encoding Rtype instructions such as add and sub. Please note the difference of the operand order
 * when an instruction is encoded in the instruction word
 *
 * "add, rd, rs, rt"
 * "sub, rd, rs, rt".
 */
struct RTypeALUInstruction {
    unsigned int unused:11;
    unsigned int Rd:5;
    unsigned int Rt:5;
    unsigned int Rs:5;
    unsigned int func:6;
};

/**
 * for encoding lw, sw, beq and addi instructions. Please note the difference of the operand order when an
 * instruction is encoded.
 *
 * lw   is written as "lw,   rt, rs, imm" in the source code. rt is the destination register for the memory data
 * sw   is written as "sw,   rt, rs, imm" in the source code. rt is the register that supplies data to the memory
 * beq  is written as "beq,  rt, rs, imm" in the source code.
 * addi is written as "addi, rt, rs, imm" in the source code. rt is the destination register for the result
 */
struct ITypeInstruction {
    int Imm:16;
    unsigned int Rt:5;
    unsigned int Rs:5;
    unsigned int func:6;
};

/**
 * "j imm" format
 */
struct JUMPInstruction {
    int Imm:26;
    unsigned int func:6;
};

int assembler(char * instruction) {
    char *func = strtok (instruction,",");
    func = removeWhite(func);

    struct RTypeALUInstruction rtypeALU;
    int isRType = 0;
    struct ITypeInstruction itypeLWSWBEQADDI;
    int isIType = 0;
    struct JUMPInstruction jump;
    int isJump = 0;

    if (strcasecmp(func, "ADD")==0) {
        rtypeALU.func = ADD;
        isRType = 1;
    } else if (strcasecmp(func, "SUB")==0) {
        rtypeALU.func = SUB;
        isRType = 1;
    } else if (strcasecmp(func, "LW")==0) {
        itypeLWSWBEQADDI.func = LW;
        isIType = 1;
    } else if (strcasecmp(func, "SW")==0) {
        itypeLWSWBEQADDI.func = SW;
        isIType = 1;
    } else if (strcasecmp(func, "BEQ")==0) {
        itypeLWSWBEQADDI.func = BEQ;
        isIType = 1;
    } else if (strcasecmp(func, "ADDI")==0) {
        itypeLWSWBEQADDI.func = ADDI;
        isIType = 1;
    } else if (strcasecmp(func, "J")==0) {
        jump.func = J;
        isJump = 1;
    } else {
        printf("Unrecognized instruction: %s, ignore.\n", func);
        return 0;
    }

    if (isRType) {
        char * Rd = strtok (NULL, ",");
        Rd = remove$s(removeWhite(Rd));
        rtypeALU.Rd = atoi(Rd);

        char * Rs = strtok (NULL, ",");
        Rs = remove$s(removeWhite(Rs));
        rtypeALU.Rs = atoi(Rs);

        char * Rt = strtok (NULL, ",");
        Rt = remove$s(removeWhite(Rt));
        rtypeALU.Rt = atoi(Rt);

        rtypeALU.unused = 0;

        /* use cast to convert to an instruction word */
        int instructionWord = *(int*)(&rtypeALU);
        return instructionWord;
    } else if (isIType) {
        char *Rt = strtok(NULL, ",");
        Rt = remove$s(removeWhite(Rt));
        itypeLWSWBEQADDI.Rt = atoi(Rt);

        char *Rs = strtok(NULL, ",");
        Rs = remove$s(removeWhite(Rs));
        itypeLWSWBEQADDI.Rs = atoi(Rs);

        char * imm = strtok (NULL, ",");
        imm = remove$s(removeWhite(imm));
        itypeLWSWBEQADDI.Imm = atoi(imm);

        /* use cast to convert to an instruction word */
        int instructionWord = *(int*)(&itypeLWSWBEQADDI);
        return instructionWord;
    } else if (isJump) {
        char * imm = strtok (NULL, ",");
        imm = remove$s(removeWhite(imm));
        jump.Imm = atoi(imm);

        /* use cast to convert to an instruction word */
        int instructionWord = *(int*)(&jump);
        return instructionWord;
    } else {
        return 0;
    }
}

void decoder(int instrWord, char *func, char * RsSelect, char * RtSelect, char * RdSelect, int * immField) {
    struct AnyInstruction instr = *(struct AnyInstruction*) &instrWord;
    switch (instr.func) {
        case ADD:
        case SUB: {
            struct RTypeALUInstruction rtypeALU = *(struct RTypeALUInstruction *) &instrWord;
            *func = rtypeALU.func;
            *RsSelect = rtypeALU.Rs;
            *RtSelect = rtypeALU.Rt;
            *RdSelect = rtypeALU.Rd;
            break;
        }
        case LW:
        case SW:
        case BEQ:
        case ADDI:
        {
            struct ITypeInstruction lwSWBEQ = *(struct ITypeInstruction *) &instrWord;
            *func = lwSWBEQ.func;
            *RsSelect = lwSWBEQ.Rs;
            *RtSelect = lwSWBEQ.Rt;
            *immField = lwSWBEQ.Imm;
            break;
        }
        case J:
        {
            struct JUMPInstruction jump = *(struct JUMPInstruction *) &instrWord;
            *func = jump.func;
            *immField = jump.Imm;
            break;
        }
    }
}

void disassembler (int instrWord) {
    struct AnyInstruction instr = *(struct AnyInstruction*) &instrWord;
    char * funcName;
    switch (instr.func) {
        case ADD: {
            funcName = "ADD";
            struct RTypeALUInstruction rtypeALU = *(struct RTypeALUInstruction *) &instrWord;
            printf("\t0x%08x: %s, $s%d, $s%d, $s%d\n", instrWord, funcName, rtypeALU.Rd, rtypeALU.Rs, rtypeALU.Rt);
            break;
        }
        case SUB: {
            funcName = "SUB";
            struct RTypeALUInstruction rtypeALU = *(struct RTypeALUInstruction *) &instrWord;
            printf("\t0x%08x: %s, $s%d, $s%d, $s%d\n", instrWord, funcName, rtypeALU.Rd, rtypeALU.Rs, rtypeALU.Rt);
            break;
        }
        case LW: {
            funcName = "LW";
            struct ITypeInstruction lwSWBEQ = *(struct ITypeInstruction *) &instrWord;
            printf("\t0x%08x: %s, $s%d, $s%d, %d\n", instrWord, funcName, lwSWBEQ.Rt, lwSWBEQ.Rs, lwSWBEQ.Imm);
            break;
        }
        case SW: {
            funcName = "SW";
            struct ITypeInstruction lwSWBEQ = *(struct ITypeInstruction *) &instrWord;
            printf("\t0x%08x: %s, $s%d, $s%d, %d\n", instrWord, funcName, lwSWBEQ.Rt, lwSWBEQ.Rs, lwSWBEQ.Imm);
            break;
        }
        case BEQ:
        {
            funcName = "BEQ";
            struct ITypeInstruction lwSWBEQ = *(struct ITypeInstruction *) &instrWord;
            printf("\t0x%08x: %s, $s%d, $s%d, %d\n", instrWord, funcName, lwSWBEQ.Rt, lwSWBEQ.Rs, lwSWBEQ.Imm);
            break;
        }
        case ADDI:
        {
            funcName = "ADDI";
            struct ITypeInstruction lwSWBEQ = *(struct ITypeInstruction *) &instrWord;
            printf("\t0x%08x: %s, $s%d, $s%d, %d\n", instrWord, funcName, lwSWBEQ.Rt, lwSWBEQ.Rs, lwSWBEQ.Imm);
            break;
        }
        case J:
        {
            funcName = "J";
            struct JUMPInstruction jump = *(struct JUMPInstruction *) &instrWord;
            printf("\t0x%08x: %s, %d\n", instrWord, funcName, jump.Imm);
            break;
        }
    }
}

#define MAXCHAR 1024
int main(int argc, char * argv[]) {
    /* fileName should be provided as the first parameter of the program */
    if (argc != 2) {
        printf("Usage: assembler_decoder <fileName>\n");
        return 1;
    }
    char lingBuffer[MAXCHAR];

    FILE *srcFile = fopen(argv[1], "r");
    if (srcFile == NULL){
        printf("Could not open file %s",argv[1]);
        return 1;
    }
    /* first step processing: find all the labels and record its address, then remove all
     * the labels (e.g. loop: ), the comment lines and empty lines. Write the result to a
     * processed file
     */
    char binFileName[strlen(argv[1])+8];
    sprintf(binFileName, "%s%s", argv[1], ".bin\0");
    FILE *binFile = fopen(binFileName, "w");
    while (fgets(lingBuffer, MAXCHAR, srcFile) != NULL) {
        char * ptr = lingBuffer;
        /* ignore comment line which starts with #, and blank line. */
        while(*ptr==' ' || *ptr=='\t') ptr++; // skip whitespaces
        if (*ptr == '#') continue; /* comment line, continue */
        if(*ptr=='\r' || *ptr=='\n') continue;

        printf("%s", ptr);

        int iw = assembler(ptr);
        fprintf(binFile, "%08x\n", iw);
        disassembler(iw);
    }
    fclose(srcFile);
    fclose(binFile);
    return 0;
}

/**
 * This function remove the leading and traling white space of a string
 * @param str
 * @return
 */
char *removeWhite(char *str)
{
    /* remove leanding space */
    while( isspace(*str) )
    {
        str++;
    }
    /* remove trailing space */
    char *end = str + strlen(str) -1;
    while( (end > str) && (isspace(*end)) )
    {
        end--;
    }

    *(end +1) = '\0';
    return str;
}

/**
 * This function remove the leading $s of an operand and it should return a string of a pure number
 * @param operand
 * @return
 */
char * remove$s(char * operand) {
    short index = 0;
    while(operand[index] == '$' || operand[index] == 's')
    {
        index++;
    }

    return operand+index;
}
