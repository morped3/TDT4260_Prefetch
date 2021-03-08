/*
 * A sample prefetcher which does sequential one-block lookahead.
 * This means that the prefetcher fetches the next block _after_ the one that
 * was just accessed. It also ignores requests to blocks already in the cache.
 */

#include "interface.hh"


void prefetch_init(void)
{
    /* Called before any calls to prefetch_access. */
    /* This is the place to initialize data structures. */

    //DPRINTF(HWPrefetch, "Initialized sequential-on-access prefetcher\n");
}

void prefetch_access(AccessStat stat)
{
    /* pf_addr is now an address within the _next_ cache block */
    Addr pf_addr = stat.mem_addr + BLOCK_SIZE;

    /*
     * Issue a prefetch request if a demand miss occured,
     * and the block is not already in cache.
     */
    if (stat.miss && !in_cache(pf_addr) && !in_mshr_queue(pf_addr)) {
        issue_prefetch(pf_addr);
    }
    else if(!stat.miss && get_prefetch_bit(stat.mem_addr)) {
        if (!in_cache(pf_addr) && !in_mshr_queue(pf_addr)) {
            issue_prefetch(pf_addr);
        }
        clear_prefetch_bit(stat.mem_addr);
    }
}

void prefetch_complete(Addr addr) {
    /*
     * Called when a block requested by the prefetcher has been loaded.
     */
     set_prefetch_bit(addr);
}

/* Is the prefetch bit set for the cache block corresponding to addr?
extern "C" int get_prefetch_bit(Addr addr);

 Set the prefetch bit for the cache block corresponding to addr.
extern "C" void set_prefetch_bit(Addr addr);

Clear the prefetch bit for the cache block corresponding to addr.
extern "C" void clear_prefetch_bit(Addr addr);  */
