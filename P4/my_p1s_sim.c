/*
 * Project 1
 * EECS 370 LC-2K Instruction-level simulator
 *
 * Make sure to NOT modify printState or any of the associated functions
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//DO NOT CHANGE THE FOLLOWING DEFINITIONS 

// Machine Definitions
#define MEMORYSIZE 65536 /* maximum number of words in memory (maximum number of lines in a given file)*/
#define NUMREGS 8 /*total number of machine registers [0,7]*/

// File Definitions
#define MAXLINELENGTH 1000 /* MAXLINELENGTH is the max number of characters we read */

typedef struct 
stateStruct {
    int pc;
    int mem[MEMORYSIZE];
    int reg[NUMREGS];
    int numMemory;
} stateType;

void printState(stateType *);

static inline int convertNum(int32_t);
int memory_len;
int opcode;
int reg_a;
int reg_b;
int reg_dest;
int offset;
int counter;
int instruct;

extern void cache_init(int blockSize, int numSets, int blocksPerSet);
extern int cache_access(int addr, int write_flag, int write_data);
extern void printStats();
static stateType state;
static int num_mem_accesses = 0;

int mem_access(int addr, int write_flag, int write_data) {
    ++num_mem_accesses;
    if (write_flag) {
        state.mem[addr] = write_data;
        if(state.numMemory <= addr) {
            state.numMemory = addr + 1;
        }
    }
    return state.mem[addr];
}
int get_num_mem_accesses(){
	return num_mem_accesses;
}


int 
main(int argc, char **argv)
{
    char line[MAXLINELENGTH];
    //stateType state;
    FILE *filePtr;

    if ( argc != 5) {
        printf("Usage: %s program.mc blockSizeInWords numberOfSets blocksPerSet\n", argv[0]);
        exit(1);
    }

    char* program_file = argv[1];
    int blockSizeInWords = atoi(argv[2]);
    int numberOfSets = atoi(argv[3]);
    int blocksPerSet = atoi(argv[4]);

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s , please ensure you are providing the correct path", argv[1]);
        perror("fopen");
        exit(2);
    }

    // INIT CACHE
    cache_init(blockSizeInWords, numberOfSets, blocksPerSet);

    /* read the entire machine-code file into memory */
    for (state.numMemory=0; fgets(line, MAXLINELENGTH, filePtr) != NULL; state.numMemory++) {
		    if (state.numMemory >= MEMORYSIZE) {
			      fprintf(stderr, "exceeded memory size\n");
			      exit(2);
		    }
		    if (sscanf(line, "%x", state.mem+state.numMemory) != 1) {
			      fprintf(stderr, "error in reading address %d\n", state.numMemory);
			      exit(2);
		    }
		    printf("memory[%d]=0x%08X\n", state.numMemory, state.mem[state.numMemory]);
    }

    memory_len = state.numMemory;

    printState(&state);
    counter = 0;

    for (state.pc = 0; state.pc < memory_len; state.pc++) {
        counter++;
        if (state.pc != 0) {
            printState(&state);
        }
        
        //Instruction Fetch
        instruct = cache_access(state.pc, 0, 0);
        opcode = (instruct & (0b111 << 22)) >> 22;

        //R_type
        if ((opcode == 0b000) || (opcode == 0b001)) {
            reg_a = (instruct & (0b111 << 19)) >> 19;
            reg_b = (instruct & (0b111 << 16)) >> 16; 
            reg_dest = (instruct & (0b111));
            if (opcode == 0b000) {
                state.reg[reg_dest] = state.reg[reg_a] + state.reg[reg_b];
            } else {
                state.reg[reg_dest] = ~(state.reg[reg_a] | state.reg[reg_b]);
            }
        } 
        if ((opcode == 0b010) || (opcode == 0b011) || (opcode == 0b100)) {
            reg_a = (instruct & (0b111 << 19)) >> 19;
            reg_b = (instruct & (0b111 << 16)) >> 16;
            offset = convertNum(instruct & (0xFFFF));
            if (opcode == 0b010) {
                state.reg[reg_b] = cache_access(state.reg[reg_a] + offset, 0, 0);
                //state.reg[reg_b] = state.mem[state.reg[reg_a] + offset];
            } else  if (opcode == 0b011) {
                cache_access(reg_a + offset, 1, state.reg[reg_b]);
                //state.mem[reg_a + offset] = state.reg[reg_b];
            } else {
                if (state.reg[reg_a] == state.reg[reg_b]) {
                    state.pc += offset;
                }
            }
        }  
        if (opcode == 0b101) {
            reg_a = (instruct & (0b111 << 19)) >> 19;
            reg_b = (instruct & (0b111 << 16)) >> 16;
            state.reg[reg_b] = (state.pc + 1);
            state.pc = state.reg[reg_a];
        }   
        if (opcode == 0b110) {
            printf("machine halted\ntotal of %d instructions executed\nfinal state of machine:\n", counter);
            state.pc += 1;
            printState(&state);
            return EXIT_SUCCESS;
        }   
    }
    
    printStats();
    printState(&state);
    //Your code ends here! 

    return(0);
}

/*
* DO NOT MODIFY ANY OF THE CODE BELOW. 
*/

void printState(stateType *statePtr) {
    int i;
    printf("\n@@@\nstate:\n");
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
    for (i=0; i<statePtr->numMemory; i++) {
        printf("\t\tmem[ %d ] 0x%08X\n", i, statePtr->mem[i]);
    }
    printf("\tregisters:\n");
	  for (i=0; i<NUMREGS; i++) {
	      printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
	  }
    printf("end state\n");
}

// convert a 16-bit number into a 32-bit Linux integer
static inline int convertNum(int num) 
{
    return num - ( (num & (1<<15)) ? 1<<16 : 0 );
}

/*
* Write any helper functions that you wish down here. 
*/
