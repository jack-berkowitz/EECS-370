/**
 * Project 2
 * LC-2K Linker
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAXSIZE 500
#define MAXLINELENGTH 1000
#define MAXFILES 6

static inline void printHexToFile(FILE *, int);

typedef struct FileData FileData;
typedef struct SymbolTableEntry SymbolTableEntry;
typedef struct RelocationTableEntry RelocationTableEntry;
typedef struct CombinedFiles CombinedFiles;

struct SymbolTableEntry {
	char label[7];
	char location;
	unsigned int offset;
};

struct RelocationTableEntry {
    unsigned int file;
	unsigned int offset;
	char inst[6];
	char label[7];
};

struct FileData {
	unsigned int textSize;
	unsigned int dataSize;
	unsigned int symbolTableSize;
	unsigned int relocationTableSize;
	unsigned int textStartingLine; // in final executable
	unsigned int dataStartingLine; // in final executable
	int text[MAXSIZE];
	int data[MAXSIZE];
	SymbolTableEntry symbolTable[MAXSIZE];
	RelocationTableEntry relocTable[MAXSIZE];
};

struct CombinedFiles {
	unsigned int textSize;
	unsigned int dataSize;
	unsigned int symbolTableSize;
	unsigned int relocationTableSize;
	int text[MAXSIZE * MAXFILES];
	int data[MAXSIZE * MAXFILES];
	SymbolTableEntry symbolTable[MAXSIZE * MAXFILES];
	RelocationTableEntry relocTable[MAXSIZE * MAXFILES];
};

int main(int argc, char *argv[]) {
	char *inFileStr, *outFileStr;
	FILE *inFilePtr, *outFilePtr; 
	unsigned int i, j, z, g, h, dist_counter, og_offset, og_text, y, found, total_size_for_stack;

    if (argc <= 2 || argc > 8 ) {
        printf("error: usage: %s <MAIN-object-file> ... <object-file> ... <output-exe-file>, with at most 5 object files\n",
				argv[0]);
		exit(1);
	}

	outFileStr = argv[argc - 1];

	outFilePtr = fopen(outFileStr, "w");
	if (outFilePtr == NULL) {
		printf("error in opening %s\n", outFileStr);
		exit(1);
	}

	FileData files[MAXFILES];

  // read in all files and combine into a "master" file
	for (i = 0; i < argc - 2; ++i) {
		inFileStr = argv[i+1];

		inFilePtr = fopen(inFileStr, "r");
		printf("opening %s\n", inFileStr);

		if (inFilePtr == NULL) {
			printf("error in opening %s\n", inFileStr);
			exit(1);
		}

		char line[MAXLINELENGTH];
		unsigned int textSize, dataSize, symbolTableSize, relocationTableSize;

		// parse first line of file
		fgets(line, MAXSIZE, inFilePtr);
		sscanf(line, "%d %d %d %d",
				&textSize, &dataSize, &symbolTableSize, &relocationTableSize);

		files[i].textSize = textSize;
		files[i].dataSize = dataSize;
		files[i].symbolTableSize = symbolTableSize;
		files[i].relocationTableSize = relocationTableSize;

		// read in text section
		int instr;
		for (j = 0; j < textSize; ++j) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			instr = strtol(line, NULL, 0);
			files[i].text[j] = instr;
		}

		// read in data section
		int data;
		for (j = 0; j < dataSize; ++j) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			data = strtol(line, NULL, 0);
			files[i].data[j] = data;
		}

		// read in the symbol table
		char label[7];
		char type;
		unsigned int addr;
		for (j = 0; j < symbolTableSize; ++j) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			sscanf(line, "%s %c %d",
					label, &type, &addr);
			files[i].symbolTable[j].offset = addr;
			strcpy(files[i].symbolTable[j].label, label);
			files[i].symbolTable[j].location = type;
		}

		// read in relocation table
		char opcode[7];
		for (j = 0; j < relocationTableSize; ++j) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			sscanf(line, "%d %s %s",
					&addr, opcode, label);
			files[i].relocTable[j].offset = addr;
			strcpy(files[i].relocTable[j].inst, opcode);
			strcpy(files[i].relocTable[j].label, label);
			files[i].relocTable[j].file	= i;
		}
		fclose(inFilePtr);
	} // end reading files

	// *** INSERT YOUR CODE BELOW ***
	//    Begin the linking process
	//    Happy coding!!!
	total_size_for_stack = 0;
	for (i = 0; i < 6; i++) {
		total_size_for_stack += files[i].textSize + files[i].dataSize;
	} 

	//go thru files
	for (i = 0; i < 6; i++) {
		//for the reolcation table
		for (j = 0; j < files[i].relocationTableSize; j++) {
			//und = 0;
			dist_counter = 0;
			found = 0;
			if (isupper(files[i].relocTable[j].label[0])) {
				// if label is upper
				for (z = 0; z < files[i].symbolTableSize; z++) {
					if ((files[i].symbolTable[z].location == 'U') && (!strcmp(files[i].symbolTable[z].label,files[i].relocTable[j].label))) {
						// Undefined in current file - need to fetch
						//und = 1;
						for (g = 0; g < 6; g++) {
							for (h = 0; h < files[g].symbolTableSize; h++) {
								// go through files and find the label
								if ((!strcmp(files[g].symbolTable[h].label, files[i].symbolTable[z].label)) && ((files[g].symbolTable[h].location == 'D') || ((files[g].symbolTable[h].location == 'T')))) {
									if (!strcmp(files[g].symbolTable[h].label, "Stack")) {
										printf("Stack defined. Exiting\n");
										return(-1);
									}
									if (found == 1) {
										printf("%s is defined twice: Ignored\n", files[g].symbolTable[h].label);
									}
									if (files[g].symbolTable[h].location == 'D') {
										for (y = 0; y < 6; y++) {
											dist_counter += files[y].textSize;
										}
										for (y = 0; y < g; y++) {
											dist_counter += files[y].dataSize;
										}
										if ((!strcmp(files[i].relocTable[j].inst, "lw")) || (!strcmp(files[i].relocTable[j].inst, "sw"))) {
											// belongs in txt of orginal file
											// in data of searched file
											files[i].text[files[i].relocTable[j].offset] += dist_counter + files[g].symbolTable[h].offset;
										} else {
											//belongs in data of orginal file
											// in data of searched file
											files[i].data[files[i].relocTable[j].offset] += dist_counter + files[g].symbolTable[h].offset;
										}
										found = 1;
									} else {
										for (y = 0; y < g; y++) {
											dist_counter += files[y].textSize;
										}
										if ((!strcmp(files[i].relocTable[j].inst, "lw")) || (!strcmp(files[i].relocTable[j].inst, "sw"))) {
											// belongs in txt of orginal file
											files[i].text[files[i].relocTable[j].offset] += dist_counter + files[g].symbolTable[h].offset;
										} else {
											// belongs in data of orginal file
											files[i].data[files[i].relocTable[j].offset] += dist_counter + files[g].symbolTable[h].offset;
										}
										found = 1;
									}
								}
							}
							if ((g == 5) && (strcmp(files[i].relocTable[j].label, "Stack"))) {
									printf("File:%d, Relocation Table:%d is undefined\n", i, j);
									return(-1);
							}
						}
						break;
					} else if ((files[i].symbolTable[z].location == 'T') && (!strcmp(files[i].symbolTable[z].label,files[i].relocTable[j].label))) {
						for (g = 0; g < i; g++) {
							dist_counter += files[g].dataSize + files[g].textSize;
						}
						dist_counter += files[i].symbolTable[z].offset;
						files[i].text[files[i].symbolTable[z].offset] += dist_counter;
						break;
					} else if ((files[i].symbolTable[z].location == 'D') && (!strcmp(files[i].symbolTable[z].label,files[i].relocTable[j].label))) {
						for (g = 0; g < i; g++) {
							dist_counter += files[g].dataSize + files[g].textSize;
						}
						dist_counter += files[i].symbolTable[z].offset + files[i].textSize;
						files[i].data[files[i].symbolTable[z].offset] += dist_counter;
						break;
					}
				}
				if (!strcmp(files[i].relocTable[j].label, "Stack")) {
					files[i].text[files[i].relocTable[j].offset] = total_size_for_stack;
				}
			} else {
				// Label in relocation table is a local
				if ((!strcmp(files[i].relocTable[j].inst, "lw")) || (!strcmp(files[i].relocTable[j].inst, "sw"))) {
					og_offset = (0xFFFF & files[i].text[files[i].relocTable[j].offset]);
					//og_text = files[i].data[og_offset-files[i].textSize];
					if (og_offset > (files[i].textSize - 1)) {
						// After all the text --> a .fill
						for (g = 0; g < 6; g++) {
							dist_counter += files[g].textSize;
						}
						for (g=0; g < i; g++) {
							dist_counter += files[g].dataSize;
						}
						dist_counter += files[i].relocTable[j].offset; // dist_counter holds the location of the .fill
					} else {
						// In the text --> a function
						for (g=0; g < i; g++) {
							dist_counter += files[g].dataSize;
						}
						dist_counter += files[i].relocTable[j].offset; //dist_counter holds the location of the function
					}
					files[i].text[files[i].relocTable[j].offset] += (dist_counter - og_offset);
				} else {
					og_text = files[i].data[files[i].relocTable[j].offset];
					if (og_text > (files[i].textSize - 1)) {
						// Referencing another .fill
						for (g = 0; g < 6; g++) {
							dist_counter += files[g].textSize;
						}
						for (g=0; g < i; g++) {
							dist_counter += files[g].dataSize;
						}
						dist_counter += files[i].relocTable[j].offset; // dist_counter holds the location of the .fill
					} else {
						// Referencing a function in txt
						for (g=0; g < i; g++) {
							dist_counter += files[g].textSize;
						}
						dist_counter += files[i].relocTable[j].offset - 1; //dist_counter holds the location of the function
					}
					files[i].data[files[i].relocTable[j].offset] = dist_counter;
				}
			}
		}
	}

	for (i=0; i<6; i++) {
		for (g=0; g<files[i].textSize; g++) {
			printHexToFile(outFilePtr, files[i].text[g]);
		}
	}
	for (i=0; i<6; i++) {
		for (g=0; g<files[i].dataSize; g++) {
			printHexToFile(outFilePtr, files[i].data[g]);
		}
	}

} // main

// Prints a machine code word in the proper hex format to the file
static inline void 
printHexToFile(FILE *outFilePtr, int word) {
    fprintf(outFilePtr, "0x%08X\n", word);
}

