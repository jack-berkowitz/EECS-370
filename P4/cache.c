/*
 * EECS 370, University of Michigan
 * Project 4: LC-2K Cache Simulator
 * Instructions are found in the project spec.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_CACHE_SIZE 256
#define MAX_BLOCK_SIZE 256

// **Note** this is a preprocessor macro. This is not the same as a function.
// Powers of 2 have exactly one 1 and the rest 0's, and 0 isn't a power of 2.
#define is_power_of_2(val) (val && !(val & (val - 1)))

unsigned long initial_mask;
unsigned long holding_mask;
int i;
int y;
int z;
int x;
unsigned int log2_int(unsigned int n);
unsigned set_index_num;
int start_block;
int block_offset;
int curr_tag;
int check_if_full(int set_indx);
void decrement_lru(int set_indx);
int val_from_mem[MAX_BLOCK_SIZE];
int start_addr;
int free_space;
//int holding_mask;
/*
 * Accesses 1 word of memory.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads and 1 for writes.
 * write_data is a word, and is only valid if write_flag is 1.
 * If write flag is 1, mem_access does: state.mem[addr] = write_data.
 * The return of mem_access is state.mem[addr].
 */
extern int mem_access(int addr, int write_flag, int write_data);

/*
 * Returns the number of times mem_access has been called.
 */
extern int get_num_mem_accesses(void);

//Use this when calling printAction. Do not modify the enumerated type below.
enum actionType
{
    cacheToProcessor,
    processorToCache,
    memoryToCache,
    cacheToMemory,
    cacheToNowhere
};

/* You may add or remove variables from these structs */
typedef struct blockStruct
{
    int data[MAX_BLOCK_SIZE];
    int dirty;
    int lruLabel;
    int tag;
    int valid;
} blockStruct;

typedef struct cacheStruct
{
    blockStruct blocks[MAX_CACHE_SIZE];
    int blockSize;
    int blockSize_in_bits;
    unsigned block_offset_mask;
    int numSets;
    int set_index_bits;
    unsigned set_index_mask;
    int blocksPerSet;
    unsigned tag_mask;
    int total_size;
    int lru_max;
    // add any variables for end-of-run stats
} cacheStruct;

/* Global Cache variable */
cacheStruct cache;

void printAction(int, int, enum actionType);
void printCache(void);

/*
 * Set up the cache with given command line parameters. This is
 * called once in main(). You must implement this function.
 */
void cache_init(int blockSize, int numSets, int blocksPerSet)
{
    if (blockSize <= 0 || numSets <= 0 || blocksPerSet <= 0) {
        printf("error: input parameters must be positive numbers\n");
        exit(1);
    }
    if (blocksPerSet * numSets > MAX_CACHE_SIZE) {
        printf("error: cache must be no larger than %d blocks\n", MAX_CACHE_SIZE);
        exit(1);
    }
    if (blockSize > MAX_BLOCK_SIZE) {
        printf("error: blocks must be no larger than %d words\n", MAX_BLOCK_SIZE);
        exit(1);
    }
    if (!is_power_of_2(blockSize)) {
        printf("warning: blockSize %d is not a power of 2\n", blockSize);
    }
    if (!is_power_of_2(numSets)) {
        printf("warning: numSets %d is not a power of 2\n", numSets);
    }
    printf("Simulating a cache with %d total lines; each line has %d words\n",
        numSets * blocksPerSet, blockSize);
    printf("Each set in the cache contains %d lines; there are %d sets\n",
        blocksPerSet, numSets);

    /********************* Initialize Cache *********************/

    initial_mask = 0xFFFFFFFF;

    // Calc block offset bits
    cache.blockSize = blockSize;
    cache.blockSize_in_bits = log2_int(blockSize);
    cache.block_offset_mask = initial_mask >> (32 - cache.blockSize_in_bits);
    holding_mask = initial_mask - cache.block_offset_mask;
    // Calc Set Index bits
    cache.numSets = numSets;
    cache.set_index_bits = log2_int(numSets);
    cache.set_index_mask = ((initial_mask >> (32 - cache.set_index_bits)) << cache.blockSize_in_bits);
    holding_mask = holding_mask - cache.set_index_mask;
    // Calc tag bits
    //cache.tag_bits = 32 - cache.set_index_bits - cache.blockSize_in_bits;
    cache.tag_mask = holding_mask;
    // Set blocks per set and LRU
    cache.blocksPerSet = blocksPerSet;
    cache.lru_max = cache.blocksPerSet;
    
    cache.total_size = cache.blockSize * cache.blocksPerSet * cache.numSets;
    
    //int i;
    //int y;
    //int z;
    //for ( i = 0; i < cache.total_size; i++) {

    //}
    return;
}

/*
 * Access the cache. This is the main part of the project,
 * and should call printAction as is appropriate.
 * It should only call mem_access when absolutely necessary.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads (fetch/lw) and 1 for writes (sw).
 * write_data is a word, and is only valid if write_flag is 1.
 * The return of mem_access is undefined if write_flag is 1.
 * Thus the return of cache_access is undefined if write_flag is 1.
 */
int cache_access(int addr, int write_flag, int write_data)
{
    // Setup bit feilds
    set_index_num = (addr & cache.set_index_mask) >> cache.blockSize_in_bits;
    block_offset = addr & cache.block_offset_mask;
    curr_tag = (addr & cache.tag_mask) >> (cache.set_index_bits + cache.blockSize_in_bits);

    for (i=0; i < cache.blocksPerSet; i++) {
        // Go through each block in the set to check if the tag exists there
        if ((cache.blocks[i + (set_index_num * cache.blocksPerSet)].tag == curr_tag) && (cache.blocks[i + (set_index_num * cache.blocksPerSet)].valid == 1)) { // AND VALID
            if (write_flag == 0){
                // Hit with matching tag and reading
                printAction(addr, 1, cacheToProcessor);
                // Return the value at that B.O.
                return cache.blocks[i + (set_index_num * cache.blocksPerSet)].data[block_offset];

                // Set LRU to highest number and decrement all in the set
                cache.blocks[i + (set_index_num * cache.blocksPerSet)].lruLabel = cache.lru_max;
                decrement_lru(set_index_num);
            } else {
                // Hit with matching tag and writing
                printAction(addr, 1, processorToCache);
                // write data into correct B.O
                cache.blocks[i + (set_index_num * cache.blocksPerSet)].data[block_offset] = write_data;

                // Set LRU to highest number and decrement all in the set
                cache.blocks[i + (set_index_num * cache.blocksPerSet)].lruLabel = cache.lru_max;
                decrement_lru(set_index_num);

                // Set the dirty bit high
                cache.blocks[i + (set_index_num * cache.blocksPerSet)].dirty = 1;
                
                return 0;
            }
        }
    }
    // MISS - Not found in the cache
    if (write_flag == 0){
        free_space = check_if_full(set_index_num);
        start_addr = (curr_tag << (cache.set_index_bits + cache.blockSize_in_bits)) + (set_index_num << cache.blockSize_in_bits);
        if (free_space == -1) {
            // Cache set is full 
            for (y=0; y<cache.blocksPerSet; y++) {
                if (cache.blocks[y + (set_index_num * cache.blocksPerSet)].lruLabel == 0){
                    // The LRU to replace
                    if (cache.blocks[y + (set_index_num * cache.blocksPerSet)].dirty == 1){
                        // Dirty --> need to drop it and write it to mem 
                        printAction(((cache.blocks[y + (set_index_num * cache.blocksPerSet)].tag << (cache.set_index_bits + cache.blockSize_in_bits)) + (set_index_num << cache.blockSize_in_bits)), cache.blockSize, cacheToMemory);
                        for (z=0; z<cache.blockSize; z++){
                            mem_access(((cache.blocks[y + (set_index_num * cache.blocksPerSet)].tag << (cache.set_index_bits + cache.blockSize_in_bits)) + (set_index_num << cache.blockSize_in_bits)) + z, 1, cache.blocks[y + (set_index_num * cache.blocksPerSet)].data[z]);
                        }
                        cache.blocks[y + (set_index_num * cache.blocksPerSet)].dirty = 0;
                    } else {
                        printAction(((cache.blocks[y + (set_index_num * cache.blocksPerSet)].tag << (cache.set_index_bits + cache.blockSize_in_bits)) + (set_index_num << cache.blockSize_in_bits)), cache.blockSize, cacheToNowhere);
                    }
                    // Overwrite + add tag + update LRU
                    printAction(start_addr, cache.blockSize, memoryToCache);
                    for (z=0; z<cache.blockSize; z++){
                        cache.blocks[y + (set_index_num * cache.blocksPerSet)].data[z] = mem_access(start_addr + z, 0, 0);
                    }
                    cache.blocks[y + (set_index_num * cache.blocksPerSet)].tag = curr_tag;
                    cache.blocks[y + (set_index_num * cache.blocksPerSet)].lruLabel = cache.lru_max;
                    decrement_lru(set_index_num);

                    printAction(addr, 1, cacheToProcessor);
                    return cache.blocks[y + (set_index_num * cache.blocksPerSet)].data[block_offset];
                }
            }
        } else {
            // Cache set has space - A miss when trying to read --> Go get from mem

            // Go get from mem
            printAction(start_addr, cache.blockSize, memoryToCache);
            // Get a blk worth of data from mem at the correct addr
            for (y=0; y<cache.blockSize; y++) {
                val_from_mem[y] = mem_access(start_addr + y, 0, 0);
            }

            // Read it into the correct blk + valid bit high
            for (y=0; y<cache.blockSize; y++) {
                cache.blocks[free_space + (set_index_num * cache.blocksPerSet)].data[y] = val_from_mem[y];
            }
            cache.blocks[free_space + (set_index_num * cache.blocksPerSet)].valid = 1;

            // Set LRU to highest number and decrement all in the set
            cache.blocks[free_space + (set_index_num * cache.blocksPerSet)].lruLabel = cache.lru_max;
            decrement_lru(set_index_num);

            //Give to processor
            printAction(addr, 1, cacheToProcessor);
            return cache.blocks[free_space + (set_index_num * cache.blocksPerSet)].data[block_offset];
        }
    } else {
        free_space = check_if_full(set_index_num);
        start_addr = (curr_tag << (cache.set_index_bits + cache.blockSize_in_bits)) + (set_index_num << cache.blockSize_in_bits);
        if (free_space == -1) {
            // Cache set is full WRITING
            for (y=0; y<cache.blocksPerSet; y++) {
                if (cache.blocks[y + (set_index_num * cache.blocksPerSet)].lruLabel == 0){
                    // The LRU to replace
                    if (cache.blocks[y + (set_index_num * cache.blocksPerSet)].dirty == 1){
                        // Dirty --> need to drop it and write it to mem 
                        printAction(((cache.blocks[y + (set_index_num * cache.blocksPerSet)].tag << (cache.set_index_bits + cache.blockSize_in_bits)) + (set_index_num << cache.blockSize_in_bits)) , cache.blockSize, cacheToMemory);
                        for (z=0; z<cache.blockSize; z++){
                            mem_access(((cache.blocks[y + (set_index_num * cache.blocksPerSet)].tag << (cache.set_index_bits + cache.blockSize_in_bits)) + (set_index_num << cache.blockSize_in_bits)) + z, 1, cache.blocks[y + (set_index_num * cache.blocksPerSet)].data[z]);
                        }
                        cache.blocks[y + (set_index_num * cache.blocksPerSet)].dirty = 0;
                    } else {
                        printAction(((cache.blocks[y + (set_index_num * cache.blocksPerSet)].tag << (cache.set_index_bits + cache.blockSize_in_bits)) + (set_index_num << cache.blockSize_in_bits)), cache.blockSize, cacheToNowhere);
                    }

                    printAction(start_addr, cache.blockSize, memoryToCache);
                    // Bring in the intrested write blk from mem
                    for (z=0; z<cache.blockSize; z++){
                        cache.blocks[y + (set_index_num * cache.blocksPerSet)].data[z] = mem_access(start_addr + z, 0, 0);
                    }

                    // Overwrite + add tag + update LRU + drity bit
                    printAction(addr, 1, processorToCache);
                    cache.blocks[y + (set_index_num * cache.blocksPerSet)].data[block_offset] = write_data;
                    cache.blocks[y + (set_index_num * cache.blocksPerSet)].tag = curr_tag;
                    cache.blocks[y + (set_index_num * cache.blocksPerSet)].lruLabel = cache.lru_max;
                    decrement_lru(set_index_num);
                    cache.blocks[y + (set_index_num * cache.blocksPerSet)].dirty = 1;

                    return 0;
                }
            }
        } else {
            // Cache set has space - A miss when trying to WRITE --> Go get from mem

            // Go get from mem
            printAction(start_addr, cache.blockSize, memoryToCache);
            // Get a blk worth of data from mem at the correct addr
            for (y=0; y<cache.blockSize; y++) {
                val_from_mem[y] = mem_access(start_addr + y, 0, 0);
            }

            // Read it into the correct blk + valid bit high
            for (y=0; y<cache.blockSize; y++) {
                cache.blocks[free_space + (set_index_num * cache.blocksPerSet)].data[y] = val_from_mem[y];
            }
            cache.blocks[free_space + (set_index_num * cache.blocksPerSet)].valid = 1;

            // Change it acoordinly 
            printAction(addr, 1, processorToCache);
            cache.blocks[free_space + (set_index_num * cache.blocksPerSet)].data[block_offset] = write_data;

            // Set LRU to highest number and decrement all in the set
            cache.blocks[free_space + (set_index_num * cache.blocksPerSet)].lruLabel = cache.lru_max;
            decrement_lru(set_index_num);

            // Set ditry
            cache.blocks[free_space + (set_index_num * cache.blocksPerSet)].dirty = 1;
            return 0; 
        }
    }
    return 0;
}


/*
 * print end of run statistics like in the spec. **This is not required**,
 * but is very helpful in debugging.
 * This should be called once a halt is reached.
 * DO NOT delete this function, or else it won't compile.
 * DO NOT print $$$ in this function
 */
void printStats(void)
{
    printf("End of run statistics:\n");
    return;
}


unsigned int log2_int(unsigned int n) {
    unsigned int counter = 0;
    while (n > 1) {
        n = n >> 1;
        counter++;
    }
    return counter;
}

int check_if_full(int set_indx) {
    int counter;
    int start_blk;
    for (i=0; i < cache.blocksPerSet; i++) {
        // Go through each block in the set and make sure that the 
        if (cache.blocks[i + (set_indx * cache.blocksPerSet)].valid == 1) {
            counter++;
        } else {
            return i;
        }
    }
    return -1;
    //if (counter == cache.blocksPerSet) {
    //    return counter;
   // } else {
    //    return 0;
    //}
}

void decrement_lru(int set_indx) {
    for (x=0; x < cache.blocksPerSet; x++) {
        // Go through each block in the set 
        if (cache.blocks[x + (set_indx * cache.blocksPerSet)].lruLabel == 0){
            return;
        } else {
            cache.blocks[x + (set_indx * cache.blocksPerSet)].lruLabel --;
        }
    }
}

/*
 * Log the specifics of each cache action.
 *
 *DO NOT modify the content below.
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *  -    cacheToProcessor: reading data from the cache to the processor
 *  -    processorToCache: writing data from the processor to the cache
 *  -    memoryToCache: reading data from the memory to the cache
 *  -    cacheToMemory: evicting cache data and writing it to the memory
 *  -    cacheToNowhere: evicting cache data and throwing it away
 */
void printAction(int address, int size, enum actionType type)
{
    printf("$$$ transferring word [%d-%d] ", address, address + size - 1);

    if (type == cacheToProcessor) {
        printf("from the cache to the processor\n");
    }
    else if (type == processorToCache) {
        printf("from the processor to the cache\n");
    }
    else if (type == memoryToCache) {
        printf("from the memory to the cache\n");
    }
    else if (type == cacheToMemory) {
        printf("from the cache to the memory\n");
    }
    else if (type == cacheToNowhere) {
        printf("from the cache to nowhere\n");
    }
    else {
        printf("Error: unrecognized action\n");
        exit(1);
    }

}

/*
 * Prints the cache based on the configurations of the struct
 * This is for debugging only and is not graded, so you may
 * modify it, but that is not recommended.
 */
void printCache(void)
{
    int blockIdx;
    int decimalDigitsForWaysInSet = (cache.blocksPerSet == 1) ? 1 : (int)ceil(log10((double)cache.blocksPerSet));
    printf("\ncache:\n");
    for (int set = 0; set < cache.numSets; ++set) {
        printf("\tset %i:\n", set);
        for (int block = 0; block < cache.blocksPerSet; ++block) {
            blockIdx = set * cache.blocksPerSet + block;
            if(cache.blocks[set * cache.blocksPerSet + block].valid) {
                printf("\t\t[ %0*i ] : ( V:T | D:%c | LRU:%-*i | T:%i )\n\t\t%*s{",
                    decimalDigitsForWaysInSet, block,
                    (cache.blocks[blockIdx].dirty) ? 'T' : 'F',
                    decimalDigitsForWaysInSet, cache.blocks[blockIdx].lruLabel,
                    cache.blocks[blockIdx].tag,
                    7+decimalDigitsForWaysInSet, "");
                for (int index = 0; index < cache.blockSize; ++index) {
                    printf(" 0x%08X", cache.blocks[blockIdx].data[index]);
                }
                printf(" }\n");
            }
            else {
                printf("\t\t[ %0*i ] : ( V:F )\n\t\t%*s{     }\n", decimalDigitsForWaysInSet, block, 7+decimalDigitsForWaysInSet, "");
            }
        }
    }
    printf("end cache\n");
}
