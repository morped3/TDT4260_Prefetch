#include "interface.hh"
#include <stdlib.h>
#include <stdio.h>

#define INDEX_TABLE_SIZE 512
#define GHB_SIZE 1024
// Structs for Global History Table and Index table.
typedef struct {
    int32_t address;
    int16_t previous;
} GHB_Entry;

GHB_Entry *ghb;
int16_t ghb_pointer;


typedef struct {
    int32_t pc;
    int16_t index;
} index_table_entry;

index_table_entry *index_table;
int16_t it_pointer;

void prefetch_init(void)
{
    /* Called before any calls to prefetch_access. */
    /* This is the place to initialize data structures. */

    //DPRINTF(HWPrefetch, "Initialized sequential-on-access prefetcher\n");

    // allocate memory for the GHB and the key buffer.
    index_table = (index_table_entry*) calloc(sizeof(index_table_entry), INDEX_TABLE_SIZE);
    ghb = (GHB_Entry*) calloc(sizeof(GHB_Entry), GHB_SIZE);

    // init table/buffer pointers
    ghb_pointer = -1;
    it_pointer = -1;
}

void prefetch_access(AccessStat stat)
{
    int16_t index_of_entry = -1;
    int16_t index_at_entry = -1;
    int32_t pc = stat.pc;

    // Make sure we dont have duplicates in Index table, if found, use that one instead.
    for (int16_t i = 0; i < INDEX_TABLE_SIZE; i++)
    {
        if (index_table[i].pc == pc)
        {
            index_at_entry = index_table[i].index;
            index_of_entry = i;
            break;
        }
    }

    // If not in index table, we make a new entry
    if(index_at_entry == -1){
        it_pointer = (it_pointer + 1) % INDEX_TABLE_SIZE;
        index_table[it_pointer].pc = pc;
        index_table[it_pointer].index = index_at_entry;
        index_of_entry = it_pointer;
    }

    ghb_pointer = (ghb_pointer + 1) % GHB_SIZE;
    ghb[ghb_pointer].address = stat.mem_addr;
    ghb[ghb_pointer].previous = index_at_entry;

    index_table[index_of_entry].index = ghb_pointer;

    // delta correlation:
    int delta_pointer = -1;
    int delta_size = 3; // 1 for prefetch degree + 2 for match pattern
    int32_t delta_buffer[delta_size];
    int32_t delta_comparison[2];
    int32_t pf_addr = stat.mem_addr; // start as address requested, but updated if delta pattern found

    int16_t index_curr_ghb = ghb_pointer;
    int16_t index_prev_ghb = 0;

    for (int i = 0; i < 128; i++) {
      index_prev_ghb = ghb[index_curr_ghb].previous;
      if (index_prev_ghb == -1) {
        break; // get out if we are at the end of the list.
      }
      int32_t delta = ghb[index_curr_ghb].address - ghb[index_prev_ghb].address;
      delta_pointer = (delta_pointer + 1) % delta_size;
      delta_buffer[delta_pointer] = delta;

      index_curr_ghb = index_prev_ghb;


      if (i < 2) {
        delta_comparison[i] = delta_buffer[i];
      }

      if (i > delta_size-2) {
        int delta_pointer_i = delta_pointer;
        int pattern_match = 1;
        for (int k = 0; k < 2; k++)
        {
            if (delta_buffer[delta_pointer_i] != delta_comparison[1-k])
            {
                pattern_match = 0;
            }
            delta_pointer_i = (delta_pointer_i - 1 + delta_size) % delta_size;
        }
        if (pattern_match)
        {
            pf_addr += delta_buffer[delta_pointer_i];
            if (!in_cache(pf_addr) && !in_mshr_queue(pf_addr) && 0 <= pf_addr && pf_addr < MAX_PHYS_MEM_ADDR)
            {
                issue_prefetch(pf_addr);
            }
            delta_pointer_i = (delta_pointer_i - 1 + delta_size) % delta_size;
            break;
        }
      }
    }
}
void prefetch_complete(Addr addr) {
    /*
     * Called when a block requested by the prefetcher has been loaded.
     */
}
