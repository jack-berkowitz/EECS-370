/**
 * Project 1
 * Assembler code fragment for LC-2K
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//Every LC2K file will contain less than 1000 lines of assembly.
#define MAXLINELENGTH 1000

#define MAXLABELS 1000
#define MAXLABELLENGTH 7

int readAndParse(FILE *, char *, char *, char *, char *, char *);
static void checkForBlankLinesInCode(FILE *inFilePtr);
static inline int isNumber(char *);
static inline void printHexToFile(FILE *, int);
int resolveLabel(char *label);

int symbolCount = 0;

typedef struct {
    char label[MAXLABELLENGTH + 1];
    int address;
} LabelEntry;

LabelEntry symbolTable[MAXLABELS];

int
main(int argc, char **argv)
{
    char *inFileString, *outFileString;
    FILE *inFilePtr, *outFilePtr;
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
            arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];

    if (argc != 3) {
        printf("error: usage: %s <assembly-code-file> <machine-code-file>\n",
            argv[0]);
        exit(1);
    }

    inFileString = argv[1];
    outFileString = argv[2];

    inFilePtr = fopen(inFileString, "r");
    if (inFilePtr == NULL) {
        printf("error in opening %s\n", inFileString);
        exit(1);
    }

    // Check for blank lines in the middle of the code.
    checkForBlankLinesInCode(inFilePtr);

    outFilePtr = fopen(outFileString, "w");
    if (outFilePtr == NULL) {
        printf("error in opening %s\n", outFileString);
        exit(1);
    }

    // STUDENT SECTION
    int counter = 0;
    int32_t machinecode = 0;
    int reg_a = 0;
    int reg_b = 0;
    int reg_dest = 0;
    int offset_val = 0;
    int fill_val = 0;

    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
        if (label[0] != '\0') {
            strcpy(symbolTable[symbolCount].label, label);
            symbolTable[symbolCount].address = counter;
            symbolCount++;
        }  
        counter++;
    }

    rewind(inFilePtr);

    counter = 0;

    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
        // R-Type
        if (!(strcmp(opcode,"add")) || !(strcmp(opcode,"nor"))) {
            reg_a = atoi(arg0);
            reg_b = atoi(arg1);
            reg_dest = atoi(arg2);
            if (!(strcmp(opcode,"add"))) {
                machinecode = (0b000 << 22) + (reg_a << 19) + (reg_b << 16) + (reg_dest);
            } else {
                machinecode = (0b001 << 22) + (reg_a << 19) + (reg_b << 16) + (reg_dest);
            }
            printHexToFile(outFilePtr, machinecode);
        }
        
        //I-type
        if (!(strcmp(opcode,"lw")) || !(strcmp(opcode,"sw")) || !(strcmp(opcode,"beq"))) {
            reg_a = atoi(arg0);
            reg_b = atoi(arg1);
            if (!isNumber(arg2)) {
                offset_val = resolveLabel(arg2);
                if (!(strcmp(opcode,"beq"))) {
                    offset_val = (offset_val) - (1 + counter);
                    offset_val = offset_val & 0xFFFF;
                }
            } else {
                offset_val = atoi(arg2);
            }

            
            if (!(strcmp(opcode,"lw"))) {
                machinecode = (0b010 << 22) + (reg_a << 19) + (reg_b << 16) + (offset_val);
            } else if (!(strcmp(opcode,"sw"))) {
                machinecode = (0b011 << 22) + (reg_a << 19) + (reg_b << 16) + (offset_val);
            } else {
                machinecode = (0b100 << 22) + (reg_a << 19) + (reg_b << 16) + (offset_val);
            }
            printHexToFile(outFilePtr, machinecode);
        }

        //J-type
        if (!(strcmp(opcode,"jalr"))) {
            reg_a = atoi(arg0);
            reg_b = atoi(arg1);
            machinecode = (0b101 << 22) + (reg_a << 19) + (reg_b << 16);
            printHexToFile(outFilePtr, machinecode);
        }

        //O-type
        if (!(strcmp(opcode,"halt"))) {
            machinecode = (0b110 << 22);
            printHexToFile(outFilePtr, machinecode);
        } else if (!(strcmp(opcode,"noop"))) {
            machinecode = (0b111 << 22);
            printHexToFile(outFilePtr, machinecode);
        }

        //.fill
        if (!(strcmp(opcode,".fill"))) {
            if (!isNumber(arg0)) {
                fill_val = resolveLabel(arg0);
            } else {
                fill_val = atoi(arg0);
            }
            machinecode = fill_val;
            printHexToFile(outFilePtr, machinecode);
        }
        counter++;
    }
    return(0);
}

// Returns non-zero if the line contains only whitespace.
static int lineIsBlank(char *line) {
    char whitespace[4] = {'\t', '\n', '\r', ' '};
    int nonempty_line = 0;
    for(int line_idx=0; line_idx < strlen(line); ++line_idx) {
        int line_char_is_whitespace = 0;
        for(int whitespace_idx = 0; whitespace_idx < 4; ++ whitespace_idx) {
            if(line[line_idx] == whitespace[whitespace_idx]) {
                line_char_is_whitespace = 1;
                break;
            }
        }
        if(!line_char_is_whitespace) {
            nonempty_line = 1;
            break;
        }
    }
    return !nonempty_line;
}

// Exits 2 if file contains an empty line anywhere other than at the end of the file.
// Note calling this function rewinds inFilePtr.
static void checkForBlankLinesInCode(FILE *inFilePtr) {
    char line[MAXLINELENGTH];
    int blank_line_encountered = 0;
    int address_of_blank_line = 0;
    rewind(inFilePtr);

    for(int address = 0; fgets(line, MAXLINELENGTH, inFilePtr) != NULL; ++address) {
        // Check for line too long
        if (strlen(line) >= MAXLINELENGTH-1) {
            printf("error: line too long\n");
            exit(1);
        }

        // Check for blank line.
        if(lineIsBlank(line)) {
            if(!blank_line_encountered) {
                blank_line_encountered = 1;
                address_of_blank_line = address;
            }
        } else {
            if(blank_line_encountered) {
                printf("Invalid Assembly: Empty line at address %d\n", address_of_blank_line);
                exit(2);
            }
        }
    }
    rewind(inFilePtr);
}


/*
* NOTE: The code defined below is not to be modifed as it is implimented correctly.
*/

/*
 * Read and parse a line of the assembly-language file.  Fields are returned
 * in label, opcode, arg0, arg1, arg2 (these strings must have memory already
 * allocated to them).
 *
 * Return values:
 *     0 if reached end of file
 *     1 if all went well
 *
 * exit(1) if line is too long.
 */
int resolveLabel(char *label) {
    for (int i = 0; i < symbolCount; i++) {
        if (strcmp(symbolTable[i].label, label) == 0) {
            return symbolTable[i].address;
        }
    }
    printf("Error: Undefined label: %s\n", label);
    exit(1);
}

int readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0,
    char *arg1, char *arg2)
{
    char line[MAXLINELENGTH];
    char *ptr = line;

    /* delete prior values */
    label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';

    /* read the line from the assembly-language file */
    if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL) {
	/* reached end of file */
        return(0);
    }

    /* check for line too long */
    if (strlen(line) == MAXLINELENGTH-1) {
	printf("error: line too long\n");
	exit(1);
    }

    // Ignore blank lines at the end of the file.
    if(lineIsBlank(line)) {
        return 0;
    }

    /* is there a label? */
    ptr = line;
    if (sscanf(ptr, "%[^\t\n ]", label)) {
	/* successfully read label; advance pointer over the label */
        ptr += strlen(label);
    }

    /*
     * Parse the rest of the line.  Would be nice to have real regular
     * expressions, but scanf will suffice.
     */
    sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]",
        opcode, arg0, arg1, arg2);

    return(1);
}

static inline int
isNumber(char *string)
{
    int num;
    char c;
    return((sscanf(string, "%d%c",&num, &c)) == 1);
}


// Prints a machine code word in the proper hex format to the file
static inline void 
printHexToFile(FILE *outFilePtr, int word) {
    fprintf(outFilePtr, "0x%08X\n", word);
}
