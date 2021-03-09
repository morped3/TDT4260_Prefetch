#include "interface.hh"
#include <stdlib.h>
#include <stdio.h>

// For GHB sizes up to 1024, KB_SIZE+GHB_SIZE can be 1724 (8KB) : 28+10 bits per line
// For GHB sizes up to 2048, KB_SIZE+GHB_SIZE can be 1680 (8KB) : 28+11 bits per line

// Magic Numbers
#define VERBOSE 1
#define CALIBRATION_INTERVAL (2*1024)
#define KB_SIZE 512
#define GHB_SIZE 1024
#define MATCH_DEGREE 2
#define LOOKBACK_AMOUNT 64
#define PREFETCH_DEGREE_DEFAULT 1
#define STORE_MISSES_ONLY 0
#define CZONE_MODE 0
#define CZONE_BITS_DEFAULT 16
#define RATE_FACTOR 1000000

// Prototypes
void prefetcher_init();
void prefetcher_access(AccessStat stat);
void prefetcher_delta_correlate();


//=========
// Helpers
//=========

void issue_if_needed(Addr addr)
{
    if (!in_cache(addr) && !in_mshr_queue(addr) && 0 <= addr && addr < MAX_PHYS_MEM_ADDR)
    {
        issue_prefetch(addr);
    }
}

//===========
// Framework
//===========


void prefetch_init()
{
    prefetcher_init();
}

void prefetch_access(AccessStat stat)
{
    // Run prefetcher logic
    prefetcher_access(stat);
}

void prefetch_complete(Addr addr)
{
    // Tag block as prefetched
}

//============
// Prefetcher
//============

int prefetch_degree = PREFETCH_DEGREE_DEFAULT;

void prefetcher_init(void)
{
    kb_init(KB_SIZE);
    ghb_init(GHB_SIZE);
}

void prefetcher_access(AccessStat stat)
{

    // Calculate key
    KB_Key key;

        key = stat.pc;

    // Find index
    KB_Index key_i = -1;
    KB_Index index = -1;
    for (KB_Index i = 0; i < kb_size; i++)
    {
        if (kb[i].key == stat.pc)
        {
            index = kb[i].index;
            key_i = i;
            break;
        }
    }

    // Create if not found
    if (index == -1)
    {
        kb_store(key, -1);
        key_i = kb_head;
    }

    // Store miss
    ghb_store(stat.mem_addr, index);

    // Update key
    kb[key_i].index = ghb_head;

    // Run Delta Correlation
    prefetcher_delta_correlate();
}

void prefetcher_delta_correlate()
{


    // Create a buffer for deltas
    int buffer_head = -1;
    int buffer_size = prefetch_degree + MATCH_DEGREE;
    GHB_Address buffer[buffer_size];

    // For storing the ghb indexes
    GHB_Index index_current = ghb_head;
    GHB_Index index_previous = 0;

    // For storing the comparison deltas
    GHB_Address deltas[MATCH_DEGREE]; //TODO: GHB_Index better than GHB_Address???

    // Get latest address
    GHB_Address address = ghb[ghb_head].address;

    for (int i = 0; i < LOOKBACK_AMOUNT; i++)
    {
        // Get previous
        index_previous = ghb[index_current].previous;

        // Exit if invalid
        if (index_previous == -1)
        {
            break;
        }

        // Calculate delta
        GHB_Address delta = ghb[index_current].address - ghb[index_previous].address;

        // Add to buffer
        buffer_head = (buffer_head + 1) % buffer_size;
        buffer[buffer_head] = delta;

        // Store comparison deltas
        if (i < MATCH_DEGREE)
        {
            deltas[i] = buffer[i];
        }

        // Check for correlation once buffer is filled
        if (i > buffer_size - 2)
        {
            int buffer_i = buffer_head;
            int match = 1;
            for (int k = 0; k < MATCH_DEGREE; k++)
            {
                if (buffer[buffer_i] != deltas[MATCH_DEGREE-k-1])
                {
                    match = 0;
                }
                buffer_i = (buffer_i - 1 + buffer_size) % buffer_size;
            }
            if (match)
            {
                // Prefetch
                for (int k = 0; k < prefetch_degree; k++)
                {
                    address += buffer[buffer_i];
                    issue_if_needed(address);
                    buffer_i = (buffer_i - 1 + buffer_size) % buffer_size;
                }
                if (VERBOSE >= 2) printf("Prefetching blocks! (degree %d)\n", prefetch_degree);
                break;
            }
        }
    }
}
