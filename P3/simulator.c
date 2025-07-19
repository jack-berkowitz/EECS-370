/*
 * EECS 370, University of Michigan, Fall 2023
 * Project 3: LC-2K Pipeline Simulator
 * Instructions are found in the project spec: https://eecs370.github.io/project_3_spec/
 * Make sure NOT to modify printState or any of the associated functions
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Machine Definitions
#define NUMMEMORY 65536 // maximum number of data words in memory
#define NUMREGS 8 // number of machine registers

#define ADD 0
#define NOR 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5 // will not implemented for Project 3
#define HALT 6
#define NOOP 7

int i;
int clear_flag;
int corrected = 0;
int reg_a_change = 0;
int reg_a_changed;
int reg_b_change = 0;
int reg_b_changed;

const char* opcode_to_str_map[] = {
    "add",
    "nor",
    "lw",
    "sw",
    "beq",
    "jalr",
    "halt",
    "noop"
};

#define NOOPINSTR (NOOP << 22)

typedef struct IFIDStruct {
    int instr;
	int pcPlus1;
    //int branch_missed;
} IFIDType;

typedef struct IDEXStruct {
    int instr;
	int pcPlus1;
	int valA;
	int valB;
	int offset;
} IDEXType;

typedef struct EXMEMStruct {
    int instr;
	int branchTarget;
    int eq;
	int aluResult;
	int valB;
} EXMEMType;

typedef struct MEMWBStruct {
    int instr;
	int writeData;
} MEMWBType;

typedef struct WBENDStruct {
    int instr;
	int writeData;
} WBENDType;

typedef struct stateStruct {
    unsigned int numMemory;
    unsigned int cycles; // number of cycles run so far
	int pc;
	int instrMem[NUMMEMORY];
	int dataMem[NUMMEMORY];
	int reg[NUMREGS];
	IFIDType IFID;
	IDEXType IDEX;
	EXMEMType EXMEM;
	MEMWBType MEMWB;
	WBENDType WBEND;
} stateType;

static inline int opcode(int instruction) {
    return instruction>>22;
}

static inline int field0(int instruction) {
    return (instruction>>19) & 0x7;
}

static inline int field1(int instruction) {
    return (instruction>>16) & 0x7;
}

static inline int field2(int instruction) {
    return instruction & 0xFFFF;
}

// convert a 16-bit number into a 32-bit Linux integer
static inline int convertNum(int num) {
    return num - ( (num & (1<<15)) ? 1<<16 : 0 );
}

void printState(stateType*);
void printInstruction(int);
void readMachineCode(stateType*, char*);


int main(int argc, char *argv[]) {

    /* Declare state and newState.
       Note these have static lifetime so that instrMem and
       dataMem are not allocated on the stack. */

    static stateType state, newState;

    if (argc != 2) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }

    readMachineCode(&state, argv[1]);

    /* ------------ Initialize State ------------ */
    state.pc = 0;
    state.IFID.instr = NOOP << 22;
    state.IDEX.instr = NOOP << 22;
    state.EXMEM.instr = NOOP << 22;
    state.MEMWB.instr = NOOP << 22;
    state.WBEND.instr = NOOP << 22;
    for (i=0; i<8; i++) {
        state.reg[i] = 0;
    }
    /* ------------------- END ------------------ */
    newState = state;

    while (opcode(state.MEMWB.instr) != HALT) {
        printState(&state);

        newState.cycles += 1;

        /* ---------------------- IF stage --------------------- */
        if ((state.EXMEM.eq == 1) && (opcode(state.EXMEM.instr) == 4)) {
            newState.pc = state.EXMEM.branchTarget; // maybe +1 here
            newState.IFID.pcPlus1 = state.EXMEM.branchTarget;
            newState.IFID.instr = NOOPINSTR;
            //clear all others
            clear_flag = 1;
        } else {
            // NOT BRANCHING
            clear_flag = 0;
            if (opcode(state.IFID.instr) == 2) {
                // following an LW
                if ((opcode(state.instrMem[state.pc]) == 0) || (opcode(state.instrMem[state.pc]) == 1) || (opcode(state.instrMem[state.pc]) == 3) || (opcode(state.instrMem[state.pc]) == 4)) {
                    // next instruction is a ADD or NOR or SW or BEQ
                    if ((field1(state.IFID.instr)) == field0(state.instrMem[state.pc]) || (field1(state.IFID.instr)) == field1(state.instrMem[state.pc])) {
                        // either register matches destination reg of LW
                        newState.pc = state.pc;
                        newState.IFID.instr = NOOPINSTR;
                        newState.IFID.pcPlus1 = state.pc + 1;
                    } else {
                        // is a LW and follow by ADD or NOR or SW or BEQ, but not bad reg
                        newState.pc = state.pc + 1;
                        newState.IFID.pcPlus1 = state.pc + 1;
                        newState.IFID.instr = state.instrMem[state.pc];
                    }
                } else if ((opcode(state.instrMem[state.pc]) == 2) && ((field1(state.IFID.instr)) == field0(state.instrMem[state.pc]))) {
                    // LW following LW with regA going to be incorrrect
                    newState.pc = state.pc;
                    newState.IFID.instr = NOOPINSTR;
                    newState.IFID.pcPlus1 = state.pc + 1;
                } else {
                    // either LW with no data hazards or noop
                    newState.pc = state.pc + 1;
                    newState.IFID.pcPlus1 = state.pc + 1;
                    newState.IFID.instr = state.instrMem[state.pc];
                }
            } else {
                // not following a LW and not branching
                newState.pc = state.pc + 1;
                newState.IFID.pcPlus1 = state.pc + 1;
                newState.IFID.instr = state.instrMem[state.pc];
            }
        }
        /* ---------------------- ID stage --------------------- */
        if (opcode(state.IFID.instr) == 7) {
            newState.IDEX.offset = 0;
        } else {
            newState.IDEX.offset = convertNum(field2(state.IFID.instr));
        }
        newState.IDEX.pcPlus1 = state.IFID.pcPlus1;
        newState.IDEX.valA = state.reg[field0(state.IFID.instr)];
        newState.IDEX.valB = state.reg[field1(state.IFID.instr)];
        if (clear_flag == 1) {
            newState.IDEX.instr = NOOPINSTR;
        } else {
            newState.IDEX.instr = state.IFID.instr;
        }
        /* ---------------------- EX stage --------------------- */
        if ((opcode(state.WBEND.instr) == 2) && ((field1(state.WBEND.instr) == field1(state.IDEX.instr)) || (field1(state.WBEND.instr) == field0(state.IDEX.instr)))) {
            // LW 3 away - that changes field1 or regB
            corrected = 1;
            if(field1(state.WBEND.instr) == field1(state.IDEX.instr)){
                //LW has a desination register that is regB of the current argument
                reg_b_change = 1;
                reg_b_changed = state.WBEND.writeData;
            } else if (field1(state.WBEND.instr) == field0(state.IDEX.instr)) {
                //LW has a desination register that is regA of the current argument
                reg_a_change = 1;
                reg_a_changed = state.WBEND.writeData;
            }
        } 
        if (((opcode(state.WBEND.instr) == 0) || (opcode(state.WBEND.instr) == 1)) && ((field2(state.WBEND.instr) == field1(state.IDEX.instr)) || (field2(state.WBEND.instr) == field0(state.IDEX.instr)))){
            //3 instructions ahead is an ADD or NOR
            corrected = 1;
            if(field2(state.WBEND.instr) == field1(state.IDEX.instr)){
                //3rd instr. has a desination register that is regB of the current argument
                reg_b_change = 1;
                reg_b_changed = state.WBEND.writeData;
            } else if (field2(state.WBEND.instr) == field0(state.IDEX.instr)) {
                //3rd instr. has a desination register that is regA of the current argument
                reg_a_change = 1;
                reg_a_changed = state.WBEND.writeData;
            }
        }
        if ((opcode(state.MEMWB.instr) == 2) && ((field1(state.MEMWB.instr) == field1(state.IDEX.instr)) || (field1(state.MEMWB.instr) == field0(state.IDEX.instr)))) {
            // LW 2 away - that changes field1 or regB
            corrected = 1;
            if(field1(state.MEMWB.instr) == field1(state.IDEX.instr)){
                //LW has a desination register that is regB of the current argument
                reg_b_change = 1;
                reg_b_changed = state.MEMWB.writeData;
            } else if (field1(state.MEMWB.instr) == field0(state.IDEX.instr)) {
                //LW has a desination register that is regA of the current argument
                reg_a_change = 1;
                reg_a_changed = state.MEMWB.writeData;
            }
        }
        if (((opcode(state.MEMWB.instr) == 0) || (opcode(state.MEMWB.instr) == 1)) && ((field2(state.MEMWB.instr) == field1(state.IDEX.instr)) || (field2(state.MEMWB.instr) == field0(state.IDEX.instr)))) {
            //2 instructions ahead is an add or nor
            corrected = 1;
            if(field2(state.MEMWB.instr) == field1(state.IDEX.instr)){
                //2nd instr. has a desination register that is regB of the current argument
                reg_b_change = 1;
                reg_b_changed = state.MEMWB.writeData;
            } else if (field2(state.MEMWB.instr) == field0(state.IDEX.instr)) {
                //2nd instr. has a desination register that is regA of the current argument
                reg_a_change = 1;
                reg_a_changed = state.MEMWB.writeData;
            }
        }
        if (((opcode(state.EXMEM.instr) == 0) || (opcode(state.EXMEM.instr) == 1)) && ((field2(state.EXMEM.instr) == field1(state.IDEX.instr)) || (field2(state.EXMEM.instr) == field0(state.IDEX.instr)))) {
            //Next instr. is a add or nor highest prio
            corrected = 1;
            if(field2(state.EXMEM.instr) == field1(state.IDEX.instr)){
                //next instr. has a desination register that is regB of the current argument
                reg_b_change = 1;
                reg_b_changed = state.EXMEM.aluResult;
            } else if (field2(state.EXMEM.instr) == field0(state.IDEX.instr)) {
                //next instr. has a desination register that is regA of the current argument
                reg_a_change = 1;
                reg_a_changed = state.EXMEM.aluResult;
            }
        } 
        if (corrected == 0) {
            if (opcode(state.IDEX.instr) == 0) {
                // ADD inst.
                newState.EXMEM.aluResult = state.IDEX.valA + state.IDEX.valB;
            } else if (opcode(state.IDEX.instr) == 1) {
                // NOR inst.
                newState.EXMEM.aluResult = ~(state.IDEX.valA | state.IDEX.valB);
            } else if ((opcode(state.IDEX.instr) == 2) || (opcode(state.IDEX.instr) == 3)) {
                // LW or SW
                newState.EXMEM.aluResult = state.IDEX.valA + state.IDEX.offset;
            } else {
                newState.EXMEM.aluResult = 0;
            }
            newState.EXMEM.valB = state.IDEX.valB;
            if (state.IDEX.valA == state.IDEX.valB) {
                newState.EXMEM.eq = 1;
            } else {
                newState.EXMEM.eq = 0;
            }
        } else if ((reg_a_change == 1) && (reg_b_change == 0)) {
            if (opcode(state.IDEX.instr) == 0) {
                // ADD inst.
                newState.EXMEM.aluResult = reg_a_changed + state.IDEX.valB;
            } else if (opcode(state.IDEX.instr) == 1) {
                // NOR inst.
                newState.EXMEM.aluResult = ~(reg_a_changed | state.IDEX.valB);
            } else if ((opcode(state.IDEX.instr) == 2) || (opcode(state.IDEX.instr) == 3)) {
                // LW or SW
                newState.EXMEM.aluResult = reg_a_changed + state.IDEX.offset;
            } else {
                newState.EXMEM.aluResult = 0;
            }
            newState.EXMEM.valB = state.IDEX.valB;
            if (reg_a_changed == state.IDEX.valB) {
                newState.EXMEM.eq = 1;
            } else {
                newState.EXMEM.eq = 0;
            }
        } else if ((reg_a_change == 0) && (reg_b_change == 1)) {
            if (opcode(state.IDEX.instr) == 0) {
                // ADD inst.
                newState.EXMEM.aluResult = state.IDEX.valA + reg_b_changed;
            } else if (opcode(state.IDEX.instr) == 1) {
                // NOR inst.
                newState.EXMEM.aluResult = ~(state.IDEX.valA | reg_b_changed);
            } else if ((opcode(state.IDEX.instr) == 2) || (opcode(state.IDEX.instr) == 3)) {
                // LW or SW
                newState.EXMEM.aluResult = state.IDEX.valA + state.IDEX.offset;
            } else {
                newState.EXMEM.aluResult = 0;
            }
            newState.EXMEM.valB = reg_b_changed;
            if (state.IDEX.valA == reg_b_changed) {
                newState.EXMEM.eq = 1;
            } else {
                newState.EXMEM.eq = 0;
            }
        } else if ((reg_a_change == 1) && (reg_b_change == 1)) {
            if (opcode(state.IDEX.instr) == 0) {
                // ADD inst.
                newState.EXMEM.aluResult = reg_a_changed + reg_b_changed;
            } else if (opcode(state.IDEX.instr) == 1) {
                // NOR inst.
                newState.EXMEM.aluResult = ~(reg_a_changed | reg_b_changed);
            } else if ((opcode(state.IDEX.instr) == 2) || (opcode(state.IDEX.instr) == 3)) {
                // LW or SW
                newState.EXMEM.aluResult = reg_a_changed + state.IDEX.offset;
            } else {
                newState.EXMEM.aluResult = 0;
            }
            newState.EXMEM.valB = reg_b_changed;
            if (reg_a_changed == reg_b_changed) {
                newState.EXMEM.eq = 1;
            } else {
                newState.EXMEM.eq = 0;
            }
        }
        reg_a_change = 0;
        reg_a_changed = 0;
        reg_b_change = 0;
        reg_b_changed = 0;
        corrected = 0;
        newState.EXMEM.branchTarget = state.IDEX.offset + state.IDEX.pcPlus1;
        if (clear_flag == 1) {
            newState.EXMEM.instr = NOOPINSTR;
        } else {
            newState.EXMEM.instr = state.IDEX.instr;
        }
        /* --------------------- MEM stage --------------------- */
        // if branch taken here
        if ((opcode(state.EXMEM.instr) == 0) || (opcode(state.EXMEM.instr) == 1)) {
            //ADD or NOR inst.
            newState.MEMWB.writeData = state.EXMEM.aluResult;
        } else if (opcode(state.EXMEM.instr) == 2) {
            //LW  inst.
            newState.MEMWB.writeData = state.dataMem[state.EXMEM.aluResult];
        } else if (opcode(state.EXMEM.instr) == 3) {
            //SW inst.
            newState.dataMem[state.EXMEM.aluResult] = state.EXMEM.valB;
            newState.MEMWB.writeData = 0;
        } else {
            newState.MEMWB.writeData = 0;
        }
         newState.MEMWB.instr = state.EXMEM.instr;
        /* ---------------------- WB stage --------------------- */
        if ((opcode(state.MEMWB.instr) == 0) || (opcode(state.MEMWB.instr) == 1)) {
            //ADD or NOR inst.
            newState.reg[field2(state.MEMWB.instr)] = state.MEMWB.writeData;
        } else if (opcode(state.MEMWB.instr) == 2){
            // LW inst.
            newState.reg[field1(state.MEMWB.instr)] = state.MEMWB.writeData;
        }
        newState.WBEND.writeData = state.MEMWB.writeData;
        newState.WBEND.instr = state.MEMWB.instr;
        /* ------------------------ END ------------------------ */
        state = newState; /* this is the last statement before end of the loop. It marks the end
        of the cycle and updates the current state with the values calculated in this cycle */
    }
    printf("Machine halted\n");
    printf("Total of %d cycles executed\n", state.cycles);
    printf("Final state of machine:\n");
    printState(&state);
}

/*
* DO NOT MODIFY ANY OF THE CODE BELOW.
*/

void printInstruction(int instr) {
    const char* instr_opcode_str;
    int instr_opcode = opcode(instr);
    if(ADD <= instr_opcode && instr_opcode <= NOOP) {
        instr_opcode_str = opcode_to_str_map[instr_opcode];
    }

    switch (instr_opcode) {
        case ADD:
        case NOR:
        case LW:
        case SW:
        case BEQ:
            printf("%s %d %d %d", instr_opcode_str, field0(instr), field1(instr), convertNum(field2(instr)));
            break;
        case JALR:
            printf("%s %d %d", instr_opcode_str, field0(instr), field1(instr));
            break;
        case HALT:
        case NOOP:
            printf("%s", instr_opcode_str);
            break;
        default:
            printf(".fill %d", instr);
            return;
    }
}

void printState(stateType *statePtr) {
    printf("\n@@@\n");
    printf("state before cycle %d starts:\n", statePtr->cycles);
    printf("\tpc = %d\n", statePtr->pc);

    printf("\tdata memory:\n");
    for (int i=0; i<statePtr->numMemory; ++i) {
        printf("\t\tdataMem[ %d ] = 0x%08X\n", i, statePtr->dataMem[i]);
    }
    printf("\tregisters:\n");
    for (int i=0; i<NUMREGS; ++i) {
        printf("\t\treg[ %d ] = %d\n", i, statePtr->reg[i]);
    }

    // IF/ID
    printf("\tIF/ID pipeline register:\n");
    printf("\t\tinstruction = 0x%08X ( ", statePtr->IFID.instr);
    printInstruction(statePtr->IFID.instr);
    printf(" )\n");
    printf("\t\tpcPlus1 = %d", statePtr->IFID.pcPlus1);
    if(opcode(statePtr->IFID.instr) == NOOP){
        printf(" (Don't Care)");
    }
    printf("\n");
    
    // ID/EX
    int idexOp = opcode(statePtr->IDEX.instr);
    printf("\tID/EX pipeline register:\n");
    printf("\t\tinstruction = 0x%08X ( ", statePtr->IDEX.instr);
    printInstruction(statePtr->IDEX.instr);
    printf(" )\n");
    printf("\t\tpcPlus1 = %d", statePtr->IDEX.pcPlus1);
    if(idexOp == NOOP){
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\treadRegA = %d", statePtr->IDEX.valA);
    if (idexOp >= HALT || idexOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\treadRegB = %d", statePtr->IDEX.valB);
    if(idexOp == LW || idexOp > BEQ || idexOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\toffset = %d", statePtr->IDEX.offset);
    if (idexOp != LW && idexOp != SW && idexOp != BEQ) {
        printf(" (Don't Care)");
    }
    printf("\n");

    // EX/MEM
    int exmemOp = opcode(statePtr->EXMEM.instr);
    printf("\tEX/MEM pipeline register:\n");
    printf("\t\tinstruction = 0x%08X ( ", statePtr->EXMEM.instr);
    printInstruction(statePtr->EXMEM.instr);
    printf(" )\n");
    printf("\t\tbranchTarget %d", statePtr->EXMEM.branchTarget);
    if (exmemOp != BEQ) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\teq ? %s", (statePtr->EXMEM.eq ? "True" : "False"));
    if (exmemOp != BEQ) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\taluResult = %d", statePtr->EXMEM.aluResult);
    if (exmemOp > SW || exmemOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");
    printf("\t\treadRegB = %d", statePtr->EXMEM.valB);
    if (exmemOp != SW) {
        printf(" (Don't Care)");
    }
    printf("\n");

    // MEM/WB
	int memwbOp = opcode(statePtr->MEMWB.instr);
    printf("\tMEM/WB pipeline register:\n");
    printf("\t\tinstruction = 0x%08X ( ", statePtr->MEMWB.instr);
    printInstruction(statePtr->MEMWB.instr);
    printf(" )\n");
    printf("\t\twriteData = %d", statePtr->MEMWB.writeData);
    if (memwbOp >= SW || memwbOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");     

    // WB/END
	int wbendOp = opcode(statePtr->WBEND.instr);
    printf("\tWB/END pipeline register:\n");
    printf("\t\tinstruction = 0x%08X ( ", statePtr->WBEND.instr);
    printInstruction(statePtr->WBEND.instr);
    printf(" )\n");
    printf("\t\twriteData = %d", statePtr->WBEND.writeData);
    if (wbendOp >= SW || wbendOp < 0) {
        printf(" (Don't Care)");
    }
    printf("\n");

    printf("end state\n");
    fflush(stdout);
}

// File
#define MAXLINELENGTH 1000 // MAXLINELENGTH is the max number of characters we read

void readMachineCode(stateType *state, char* filename) {
    char line[MAXLINELENGTH];
    FILE *filePtr = fopen(filename, "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", filename);
        exit(1);
    }

    printf("instruction memory:\n");
    for (state->numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL; ++state->numMemory) {
        if (sscanf(line, "%x", state->instrMem+state->numMemory) != 1) {
            printf("error in reading address %d\n", state->numMemory);
            exit(1);
        }
        printf("\tinstrMem[ %d ] = 0x%08X ( ", state->numMemory, 
            state->instrMem[state->numMemory]);
        printInstruction(state->dataMem[state->numMemory] = state->instrMem[state->numMemory]);
        printf(" )\n");
    }
}
