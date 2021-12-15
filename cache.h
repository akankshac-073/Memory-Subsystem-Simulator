#ifndef CACHE_H   
#define CACHE_H

// L1 CACHE MACROS

// 2KB 4-way set-associative L1 WAY-HALTING cache
#define NUM_L1_CACHE_WAYS 4
#define NUM_L1_CACHE_SETS 16
#define NUM_L1_CACHE_BLOCK_SIZE 32
  
#define NUM_L1_CACHE_SET_INDEX_BITS 4 
#define NUM_L1_CACHE_OFFSET_BITS 5
#define NUM_L1_CACHE_HALT_TAG_BITS 4 
#define NUM_L1_CACHE_MAIN_TAG_BITS 12      

#define NUM_L1_LRU_COUNTER_BITS 8

// L2 CACHE MACROS

// 32KB 16-way set-associative L2 cache
#define NUM_L2_CACHE_WAYS 16
#define NUM_L2_CACHE_SETS 32
#define NUM_L2_CACHE_BLOCK_SIZE 64

#define NUM_L2_CACHE_SET_INDEX_BITS 5
#define NUM_L2_CACHE_OFFSET_BITS 6
#define NUM_L2_CACHE_TAG_BITS 14
 
// BIT/FLAG VALUE MACROS

// Valid bit values
#define INVALID 0
#define VALID 1

// Dirty bit values
#define CLEAN 0
#define DIRTY 1

// Read write bit values
#define READ_ONLY 0
#define READ_WRITE 1

// To specify L1 cache type
#define INSTRUCTION 0
#define DATA 1

// To specify L1 way status
#define ACTIVE 0
#define HALTED 1

// To specify the operation to be performed
#define READ_ACCESS 0
#define WRITE_ACCESS 1

// To specify access status
#define L1_CACHE_WRITE_SUCCESSFUL 256                // using numbers greater than 255 (max valid value of data that can be returned) to specify L1 cache access status 
#define L1_CACHE_WRITE_PROTECTION_EXCEPTION 257
#define L1_CACHE_MISS 258
#define L1_CACHE_MISS_PREDETERMINED 259  

// L1, L2 CACHE DATA BLOCK BYTE ENTRIES

// Each data entry in cache block is 1B = 8 bits
typedef struct {
    unsigned int data:8;                                
} data_byte;

// L1 CACHE ADT DEFINITIONS

// L1 cache entry structure 
typedef struct {
    unsigned int main_tag_bits:11;                        // First 11 bits of 15-bit cache tag bits - stored as main tag bits with each entry 
    unsigned int valid_bit:1;                             // Valid bit - set if the corresponding entry is valid
    unsigned int dirty_bit:1;                             // Dirty bit - set if the corresponding entry has been modified in this level but not updated in the next level
                                                          // (L1 cache uses write-back policy)
    unsigned int write_bit:1;                             // Read-Write permissions for the data block 
    data_byte data_blocks [NUM_L1_CACHE_BLOCK_SIZE];      // 32B data block - 1B data stored in each array element
} L1_cache_entry;

// L1 cache structures
typedef struct {
    L1_cache_entry l1_cache_entry [NUM_L1_CACHE_WAYS];     // Array of NUM_L1_CACHE_WAYS L1 cache entries per set
    unsigned int lru_counter [NUM_L1_CACHE_WAYS];          // N.log(N) bit LRU counter is maintained per set - assume log(N) bits stored in each array element N - No. of ways 
} L1_cache_sets;                                           // log -- (base 2)

typedef struct {
    unsigned int halt_tags_per_way [NUM_L1_CACHE_SETS];    // The last 4 bits of tag are maintained in a halt tag array - maintained per way (Way Halting Cache)
} Halt_tag_array;

typedef struct {
    L1_cache_sets l1_cache_sets [NUM_L1_CACHE_SETS];       // And array of NUM_L1_CACHE_SETS such sets make L1 cache
    Halt_tag_array halt_tag_array [NUM_L1_CACHE_WAYS];     // Array of NUM_L1_CACHE_WAYS halt tags -- cuz Way-Halting cache
    int way_status [NUM_L1_CACHE_WAYS];                    // Way-halting cache shuts down ways in which misses are pre-determined. Way status indicates if a particular way is HALTED or ACTIVE.
} L1_cache;

// L2 CACHE ADT DEFINITIONS

// L2 cache entry structure 
typedef struct {
    unsigned int tag:NUM_L2_CACHE_TAG_BITS;               // 14-bit cache tag bits 
    unsigned int valid_bit:1;                             // Valid bit - set if the corresponding entry is valid
    unsigned int write_bit:1;                             // Read-Write permissions for the data block 
    data_byte data_blocks [NUM_L2_CACHE_BLOCK_SIZE];      // 64B data block - 1B data stored in each array element
} L2_cache_entry;

typedef struct {
   L2_cache_entry l2_cache_entry [NUM_L2_CACHE_WAYS];     // Array of NUM_L2_CACHE_WAYS L2 cache entries per set
   unsigned int fifo_bits:4;                              // log(N) bit FIFO counter is maintained per set - assume log(N) bits stored in each array element N - No. of ways 
} L2_cache_sets;                                          // log -- (base 2)

// L2 cache structures
typedef struct {
    L2_cache_sets l2_cache_sets [NUM_L2_CACHE_SETS];                      
} L2_cache;

// L1 CACHE FUNCTION DECLARATIONS

// Initializes the L1 instruction and data cache by allocating memory for the structures and initializing all the entries
L1_cache* initialize_L1_cache (int cache_type);

// Search the L1 cache for the datablock entry corresponding to the given physical address 
unsigned int search_L1_cache (L1_cache* l1_cache, unsigned int physical_address, unsigned int write_data, int access_type);

// Predetermine the misses by searching all the halt tag arrays for the halt tag corresponding to the given physical address (in PARALLEL with set index decoding)
// So that it can HALT the ways in which a miss is predetermined in order to prevent unnecessary access, thereby saving energy
void L1_cache_way_halting_function (L1_cache* l1_cache, unsigned int halt_tag);

// Update the L1 cache with the data acquired from the next level in memory (L2 cache) - placement / random replacement
void update_L1_cache (L1_cache* l1_cache, L2_cache* l2_cache, data_byte *data, unsigned int physical_address);

// Updates the LRU counter at every access that results in L1 cache hit 
void update_L1_LRU_counter (L1_cache *l1_cache, int set_index, int way_index);

// Get the Least Recently Used entry's way index from LRU counter (way corresponding to the entry 00)
int get_L1_LRU_way_entry (L1_cache* l1_cache, int set_index);

// Print all L1 cache entries
void print_L1_cache (L1_cache *l1_cache);

// Utility function to calculate the exponent
int calc_exp (int a, int b);

// L2 CACHE FUNCTION DECLARATIONS

// Initializes the L2 cache by allocating memory for the structures and initializing all the entries
L2_cache* initialize_L2_cache ();

// Search the L2 cache for the datablock entry corresponding to the given physical address 
data_byte* search_L2_cache (L2_cache* l2_cache, unsigned int physical_address, data_byte* write_data, int access_type);

// Update the L2 cache with the data acquired from the next level in memory (Main memory) - placement / random replacement
void update_L2_cache (L2_cache* l2_cache_1, data_byte* data,unsigned int physical_address);

// Updates the FIFO counter at every access that results in L2 cache hit
void update_L2_FIFO_counter (L2_cache* l2_cache_1, int set_index);

// Get the earliest arrived (FIFO) entry's way index from FIFO counter (way corresponding to the entry 0000)
int get_FIFO_way_entry (L2_cache* l2_cache_1,int set_index);

// Print all L2 cache entries
void print_L2_cache (L2_cache *l2_cache);

#endif
