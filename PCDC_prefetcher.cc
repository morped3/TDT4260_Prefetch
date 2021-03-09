/*
 * A sample prefetcher which does sequential one-block lookahead.
 * This means that the prefetcher fetches the next block _after_ the one that
 * was just accessed. It also ignores requests to blocks already in the cache.
 */

#include "interface.hh"

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

KB_Entry *index_table;
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

    // Make sure we dont have duplicates in Index table, if found, use that one instead.
    for (int16_t i = 0; i < INDEX_TABLE_SIZE; i++)
    {
        if (index_table[i].pc == stat.pc)
        {
            index_at_entry = index_table[i].index;
            index_of_entry = i;
            break;
        }
    }

    // If not in index table, we make a new entry
    if(index_at_entry == -1){
        it_pointer = (it_pointer + 1) % INDEX_TABLE_SIZE;
        index_table[it_pointer].pc = stat.pc;
        index_table[it_pointer].index = index_at_entry;
    }

    ghb_pointer = (ghb_pointer + 1) % GHB_SIZE;
    ghb[ghb_pointer].address = stat.mem_addr;
    ghb[ghb_pointer].previous = index_at_entry;

    index_table[index_of_entry].index = ghb_pointer;




    /*
     * Issue a prefetch request if a demand miss occured,
     * and the block is not already in cache.
     */

    if (stat.miss && !in_cache(pf_addr)) {
        issue_prefetch(pf_addr);
    }
}

void prefetch_complete(Addr addr) {
    /*
     * Called when a block requested by the prefetcher has been loaded.
     */
}
