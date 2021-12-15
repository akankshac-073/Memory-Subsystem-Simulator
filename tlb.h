#ifndef TLB_H
#define TLB_H

#define MAX_FRAME_NUMBER 65535

// 8-way set-associative L1 TLB with 16 entries
#define NUM_L1_TLB_WAYS 8
#define NUM_L1_TLB_SETS 2
#define NUM_L1_TLB_SET_INDEX_BITS 1    // No. of bits required to address 2 sets -- log_2(2)

// 4-way set-associative L2 TLB with 32 entries
#define NUM_L2_TLB_WAYS 4
#define NUM_L2_TLB_SETS 8
#define NUM_L2_TLB_SET_INDEX_BITS 3    // No. of bits required to address 8 sets -- log_2(8)

// Valid bit values
#define VALID 1
#define INVALID 0

// Shared bit values
#define SHARED 1
#define NOT_SHARED 0

// TLB ADT DEFINITIONS

// L1 TLB entry structure 

typedef struct {
    unsigned int page_tag_entry:22;        // 22-bit tag entry: first 22 bits of 23-bit page number (32-bit virtual address, 9-bit offset - 512B page)
                                           // remaining 1 bit is used as set index  
    unsigned int frame_number_entry:16;    // 16-bit frame number (25-bit physical address, 9-bit offset - 512B page)
    unsigned int valid_bit:1;              // Valid bit - set if the corresponding entry is valid
    unsigned int shared_bit:1;             // Shared bit - set if the page is shared by two or more processes
} L1_TLB_entry;

// L2 TLB entry structure

typedef struct {
    unsigned int page_tag_entry:20;        // 20-bit tag entry: first 20 bits of 23-bit page number (32-bit virtual address, 9-bit offset - 512B page) 
                                           // remaining 3 bits are used as set index  
    unsigned int frame_number_entry:16;    // 16-bit frame number (25-bit physical address, 9-bit offset - 512B page)
    unsigned int valid_bit:1;              // Valid bit - set if the corresponding entry is valid
    unsigned int shared_bit:1;             // Shared bit - set if the page is shared by two or more processes
} L2_TLB_entry;

// L1 TLB structures 

typedef struct {
    L1_TLB_entry l1_tlb_entry[NUM_L1_TLB_WAYS];                // Array of NUM_L1_TLB_WAYS L1 TLB entries per set
} L1_TLB_sets;

typedef struct {
    L1_TLB_sets l1_tlb_sets[NUM_L1_TLB_SETS];                  // And array of NUM_L1_TLB_SETS such sets make L1 TLB
} L1_TLB;

// L2 TLB structures

typedef struct {
    L2_TLB_entry l2_tlb_entry[NUM_L2_TLB_WAYS];                // Array of NUM_L2_TLB_WAYS L1 TLB entries per set
    int lru_square_matrix[NUM_L2_TLB_WAYS][NUM_L2_TLB_WAYS];   // LRU square matrix is maintained per set
} L2_TLB_sets;

typedef struct {
    L2_TLB_sets l2_tlb_sets[NUM_L2_TLB_SETS];                  // And array of NUM_L2_TLB_SETS such sets make L2 TLB
} L2_TLB;

// FUNCTION DECLARATIONS

// Initializes the L1 TLB by allocating memory for the structure and initializing all the entries by marking them as invalid
L1_TLB *initialize_L1_TLB ();                                             

// Initializes the L2 TLB by y allocating memory for the structure, invalidating all the entries and initializing lru square matrix by setting all bits to 0
L2_TLB *initialize_L2_TLB ();                                             

// Search the L1 TLB for the frame number entry corresponding to the requested page 
unsigned int search_L1_TLB (L1_TLB* l1_tlb, unsigned int page_number);    

// If the L1 TLB access results in a miss - Search the L2 TLB for the frame number entry corresponding to the requested page 
unsigned int search_L2_TLB (L2_TLB* l2_tlb, unsigned int page_number);    

// Update L1 TLB with the entry acquired via page table walk - placement / random replacement
void update_L1_TLB (L1_TLB* l1_tlb, L2_TLB* l2_tlb, unsigned int pg_num, unsigned int frame_num, unsigned int shared_bit);

// Update L2 TLB with the entry replaced in L1 TLB or the entry acquired via page table walk - placement / LRU replacement 
void update_L2_TLB (L2_TLB* l2_tlb, unsigned int pg_num, unsigned int frame_num, unsigned int shared_bit);

// Get the Least Recently Used entry's way index from LRU square matrix (index of the first row with all bits 0)
int get_LRU_entry_index (L2_TLB* l2_tlb, int set_index);                  

// Flush (invalidate) all the entries, except for those corresponding to shared pages, from L1 TLB
void flush_L1_TLB (L1_TLB* l1_tlb);                                       

// Flush (invalidate) all the entries, except for those corresponding to shared pages, from L2 TLB
void flush_L2_TLB (L2_TLB* l2_tlb);

// Print all L1 TLB entries                                       
void print_L1_tlb (L1_TLB* l1_tlb);    

// Print all L2 TLB entries                                   
void print_L2_tlb (L2_TLB* l2_tlb);                                       

#endif
