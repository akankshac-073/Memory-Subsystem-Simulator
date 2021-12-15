#include <stdio.h>
#include <stdlib.h>
#include "cache.h"

// Initializes the L2 cache structures.

L2_cache* initialize_L2_cache () {
    int i = 0;
    int j = 0;

    // Create an empty L2 cache structure
    L2_cache *l2_cache;
    l2_cache = (L2_cache *) malloc (sizeof (L2_cache));
    
    // Initializing all the entries
    for (i = 0; i < NUM_L2_CACHE_SETS; i++) {
        for (j = 0; j < NUM_L2_CACHE_WAYS; j++) {
        
            // Initialize the valid bit as INVALID  
            l2_cache->l2_cache_sets[i].l2_cache_entry[j].valid_bit = INVALID;       
            
            // Initializing write bit values to READ_WRITE
            l2_cache->l2_cache_sets[i].l2_cache_entry[j].write_bit = READ_WRITE;   // L2 (unified) cache access can be READ as well as WRITE   
            
            // Initializing all FIFO counter values to 0
            l2_cache->l2_cache_sets[i].fifo_counter[j] = (1 << 4) - 1;
        }    
    }
    
    // Marking last set entry in each way as READ_ONLY (Write protected)
        for (j = 0; j < NUM_L2_CACHE_WAYS; j++)
            l2_cache->l2_cache_sets[NUM_L2_CACHE_SETS - 1].l2_cache_entry[j].write_bit = READ_ONLY;
            
    return l2_cache;    // Return pointer to the initialized cache structure
}

// Search the L2 cache for data entry corresponding to the given physical address. 
// If found, perform a read or write operation depeding on the type of access.

data_byte* search_L2_cache (L2_cache* l2_cache, unsigned int physical_address, data_byte* write_data, int access_type) {
    int i = 0;
    int j = 0;
    unsigned int tag = 0;           // L2 cache tag bits
    unsigned int set_index = 0;     // L2 cache set index bits
    unsigned int offset = 0;        // L2 cache offset bits
    unsigned int block_offset = 0;  // L2 cache block offset
    
    // Allocating memory to store the datablock to be fetched 
    data_byte* data;
    data = malloc (sizeof (data_byte) * 32);
    
    // Extracting the tag, offset and index bits from the physical address
    offset = physical_address % NUM_L2_CACHE_BLOCK_SIZE;    
    set_index = (physical_address >> NUM_L2_CACHE_OFFSET_BITS) % NUM_L2_CACHE_SETS;
    tag = physical_address >> (NUM_L2_CACHE_SET_INDEX_BITS + NUM_L2_CACHE_OFFSET_BITS); 
    
    block_offset = offset | (1 << NUM_L1_CACHE_OFFSET_BITS);      // block offset (64B - first 32B or last 32B) - location of the first byte of the 32B block
    
    // Searching the L2 cache
    for (i = 0; i < NUM_L2_CACHE_WAYS; i++) {      
                
        // If the entry is VALID and the tag value matches the main tag bits -- entry found!        
        if (l2_cache->l2_cache_sets[set_index].l2_cache_entry[i].valid_bit == VALID && l2_cache->l2_cache_sets[set_index].l2_cache_entry[i].main_tag_bits == main_tag) {
                    
            // For READ access
            if (access_type == READ_ACCESS) {
            
                // Read 32B (L1 cache block size) of data from 64B L2 cache datablock (starting location in datablock given by block offset)
                for (j = 0; j < NUM_L1_CACHE_BLOCK_SIZE; j++)
                    data = l2_cache->l2_cache_sets[set_index].l2_cache_entry[i].data_blocks[block_offset + j].data;     
 
                // Update FIFO counter corresponding to this set index
                update_L2_FIFO_counter(l2_cache, set_index, i);    
    
                return data;
            }
        }                                
        
        // For WRITE access - when WRITE permission is available
        else if (access_type == WRITE_ACCESS && l2_cache->l2_cache_sets[set_index].l2_cache_entry[i].write_bit == READ_WRITE) {
                    
            // Write 32B (L1 cache block size) of data into L2 cache datablock (starting location in datablock given by block offset)
            for (j = 0; j < NUM_L1_CACHE_BLOCK_SIZE; j++)
                l2_cache->l2_cache_sets[set_index].l2_cache_entry[i].data_blocks[block_offset + j].data = write_data;  
                                     
            // Initiate write to the corresponding block in Main Memory (write-through policy)
            // TODO update_main_memory_fn
                    
            // Update FIFO counter corresponding to this set index
            update_L2_FIFO_counter(l2_cache, set_index, i);
                        
            return write_data;    // anything other than NULL. NULL is used to indicate miss/ write exception 
        }
                
        // For WRITE access - when WRITE permission is denied --- The write access results in a WRITE PROTECTION EXCEPTION
        else if (access_type == WRITE_ACCESS && l1_cache->l1_cache_sets[set_index].l1_cache_entry[i].write_bit == READ_ONLY) {
            return NULL;
        }
    }
    
    // If the data is not found in any of the ways corresponding to the set index obtained - MISS
    return NULL;
}

// Updates the L2 cache with the datablock fetched from the next level i.e. main memory. 
// Depending on the availability of free slots an entry is PLACED/REPLACED in the L2 cache. 
// If an INVALID entry is available in any of the ways corresponding to the required set index, PLACE the new entry there. 
// Else, REPLACE any of the way entries with required set index using FIFO REPLACEMENT. 

void update_L2_cache (L2_cache* l2_cache, Main_memory* Main_memory, data_byte* fetched_data, unsigned int physical_address) {
    unsigned int tag = 0;         // L2 cache tag bits
    unsigned int set_index = 0;   // L2 cache set index bits
    unsigned int offset = 0;      // L2 cache byte offset bits
    
    // Extracting the tag, offset and index bits from the physical address
    offset = physical_address % NUM_L2_CACHE_BLOCK_SIZE;
    set_index = (physical_address >> NUM_L2_CACHE_OFFSET_BITS) % NUM_L2_CACHE_SETS;
    tag = physical_address >> (NUM_L2_CACHE_SET_INDEX_BITS + NUM_L2_CACHE_OFFSET_BITS);  
    
    int FIFO_way = 0;              // way index corresponding to first arrived entry in the set
    
    // Any block REPLACED in L2 cache needs to be updated in Main Memory immediately (Write-through Policy). 
    unsigned int write_through_address = 0;    

    int i = 0;
    int j = 0;

    // PLACEMENT: If there are INVALID entries in L1 cache corresponding to the given set index, PLACE this entry in the first INVALID entry's slot    
    
    // Looking for the first INVALID entry for given set index in L1 cache
    for (i = 0; i < NUM_L2_CACHE_WAYS; i++) {
       
        if (l2_cache->l2_cache_sets[set_index].l2_cache_entry[i].valid_bit == INVALID) {
            
            // L2 cache data block updation
            for (j = 0; j < NUM_L2_CACHE_BLOCK_SIZE; j++)
                l2_cache->l2_cache_sets[set_index].l2_cache_entry[i].data_blocks[j].data = fetched_data[j].data;
            
            // L2 cache entry updation
            l2_cache->l2_cache_sets[set_index].l2_cache_entry[i].tag = tag;  
            l2_cache->l2_cache_sets[set_index].l2_cache_entry[i].valid_bit = VALID;
            
            break;
        }        
    }
    
    // REPLACEMENT: If all entries corresponding to the given set index are VALID, check the LRU counter and replace entry corresponding to LRU way
    if (i == NUM_L2_CACHE_WAYS) {
    
        data_byte write_back_data [NUM_L2_CACHE_BLOCK_SIZE];
    
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////     
    
        // Check FIFO counter to get the FIFO way entry index
        FIFO_way = get_L2_FIFO_way_entry (l2_cache, set_index);
        
        // Initiate write back to main memory
        // Get the physical address of the dirty block to be written back to L2  
        write_through_address = (l2_cache->l2_cache_sets[set_index].l2_cache_entry[FIFO_way].tag << (NUM_L2_CACHE_SET_INDEX_BITS + NUM_L2_CACHE_OFFSET_BITS)) | (set_index << NUM_L2_CACHE_OFFSET_BITS); 
        
        // Copy datablock to be written to next level    
        for (int ii =0; ii < NUM_L2_CACHE_BLOCK_SIZE; ii++)
            write_back_data[ii].data = l2_cache->l2_cache_sets[set_index].l2_cache_entry[FIFO_way].data_blocks[ii].data;
            
        // Write the block to be replaced in L2 back to Main Memory
        // TODO update_main_memory_fn
    }
        
    // L2 cache data block updation
    for (j = 0; j < NUM_L2_CACHE_BLOCK_SIZE; j++)
        l2_cache->l2_cache_sets[set_index].l2_cache_entry[FIFO_way].data_blocks[j].data = fetched_data[j].data;
        
        // L2 cache entry updation
        l2_cache->l2_cache_sets[set_index].l2_cache_entry[FIFO_way].tag = tag; 
        l2_cache->l2_cache_sets[set_index].l2_cache_entry[FIFO_way].valid_bit = VALID;
    }
}



void update_fifo_l2_cache(L2_cache* l2_cache_1, int set_index)
{
    int i;
    for (i = 0; i < NUM_L2_CACHE_WAYS; i++) {
        if (l2_cache_1->set_array[set_index].set_entries[i].valid_bit == VALID) {
            l2_cache_1->set_array[set_index].set_entries[i].fifo_bits--;
        }        
    }
}
int get_FIFO_replacement(L2_cache* l2_cache_1,int set_index)
{
    for (int i = 0; i < NUM_L2_CACHE_WAYS; i++) {
        if (l2_cache_1->set_array[set_index].set_entries[i].fifo_bits == 0) {
            return i;
        }        
    }

}

void print_L2_cache (L2_cache *l2_cache_1) {
    for (int i = 0; i < NUM_L2_CACHE_SETS; i++) {
        if(i==28)
        {
            printf ("SET %d\n", i + 1);
            for (int j = 0; j < NUM_L2_CACHE_WAYS; j++) {
                printf(" Main Tag: %d Valid Bit: %d \n", l2_cache_1->set_array[i].set_entries[j].tag, l2_cache_1->set_array[i].set_entries[j].valid_bit);
                printf("\n\n");
            }

        }
    }   
}





