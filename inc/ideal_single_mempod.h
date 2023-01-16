#ifndef OS_TRANSPARENT_MANAGEMENT_H
#define OS_TRANSPARENT_MANAGEMENT_H
#include <map>
#include <vector>
#include <iostream>
#include <cassert>
#include <deque>
#include <unordered_map>

#include "champsim_constants.h"
#include "memory_class.h"
#include "util.h"
#include "ProjectConfiguration.h" // user file

/** @note Abbreviation:
 *  FM -> Fast memory (e.g., HBM, DDR4)
 *  SM -> Slow memory (e.g., DDR4, PCM)
 *
*/

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)

#if (IDEAL_SINGLE_MEMPOD == ENABLE)

/* MemPod Parameter Setting */
#define TIME_INTERVAL_MEMPOD_us         (50) // [us]
#define DATA_MANAGEMENT_GRANULARITY     (2048) // [B]
#define DATA_MANAGEMENT_OFFSET_BITS     (lg2(DATA_MANAGEMENT_GRANULARITY)) // [bit]
#define CACHE_LINE_SIZE                 (64) // [B]
#define SWAP_DATA_CACHE_LINES           (DATA_MANAGEMENT_GRANULARITY / CACHE_LINE_SIZE) //[lines]


/* for mea_counter_table */
#define NUMBER_MEA_COUNTER              (64)
#define MEA_COUNTER_WIDTH               uint8_t
#define MEA_COUNTER_MAX_VALUE           (15u)
#define COUNTER_DEFAULT_VALUE           (0)

/* for address_remapping_table */
#define REMAPPING_TABLE_ENTRY_WIDTH     uint64_t

/* for swapping */
#define REMAPPING_REQUEST_QUEUE_LENGTH          (4096)  // 1024/4096
#define QUEUE_BUSY_DEGREE_THRESHOLD             (0.8f)

#define INCOMPLETE_READ_REQUEST_QUEUE_LENGTH    (128)
#define INCOMPLETE_WRITE_REQUEST_QUEUE_LENGTH   (128)

class OS_TRANSPARENT_MANAGEMENT
{
public:
    uint64_t cycle = 0;
    uint64_t total_capacity;        // [B]
    uint64_t fast_memory_capacity;  // [B]
    uint64_t total_capacity_at_granularity;
    uint64_t fast_memory_capacity_at_granularity;
    uint8_t  fast_memory_offset_bit;    // address format in the data management granularity
    uint8_t  swap_size = SWAP_DATA_CACHE_LINES; // == 32

    struct mea_counter_entry{
        uint64_t segment_address;
        MEA_COUNTER_WIDTH counter_value;
    };

    /* Remapping request */
    struct RemappingRequest
    {
        uint64_t h_address_in_fm, h_address_in_sm;  // hardware address in fast and slow memories
        uint64_t p_address_in_fm, p_address_in_sm;  // physical address in fast and slow memories
        uint8_t size;   // number of cache lines to remap == 32 (2048B)
    };

    std::unordered_map<REMAPPING_TABLE_ENTRY_WIDTH, mea_counter_entry>& mea_counter_table;
    std::unordered_map<REMAPPING_TABLE_ENTRY_WIDTH, REMAPPING_TABLE_ENTRY_WIDTH>& address_remapping_table;
    std::unordered_map<REMAPPING_TABLE_ENTRY_WIDTH, REMAPPING_TABLE_ENTRY_WIDTH>& invert_address_remapping_table;

    std::deque<RemappingRequest> remapping_request_queue;
    uint64_t remapping_request_queue_congestion;

#if (PRINT_SWAP_DETAIL)
    uint64_t swap_request;
#endif // PRINT_SWAP_DETAIL

    double interval_cycle = CPU_FREQUENCY * (double)TIME_INTERVAL_MEMPOD_us / MEMORY_CONTROLLER_CLOCK_SCALE;
    double next_interval_cycle = interval_cycle;

    /* Member functions */
    OS_TRANSPARENT_MANAGEMENT(uint64_t max_address, uint64_t fast_memory_max_address);
    ~OS_TRANSPARENT_MANAGEMENT();

    // address is physical address and at byte granularity
    bool memory_activity_tracking(uint64_t address, uint8_t type, float queue_busy_degree);

    // translate the physical address to hardware address
    void physical_to_hardware_address(PACKET& packet);
    void physical_to_hardware_address(uint64_t& address);

    // detect cold data block and cycle increment
    void cold_data_detection();

    // MemPod interval swap
    void issue_interval_swaps();
    bool issue_remapping_request(RemappingRequest& remapping_request);
    bool finish_remapping_request();

    bool enqueue_remapping_request(RemappingRequest& remapping_request);
    bool cancel_not_started_remapping_request();
    
private:
    void get_hot_page_from_mea_counter(std::vector<mea_counter_entry>& hot_pages);



};

#endif  // IDEAL_SINGLE_MEMPOD

#endif  // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT
#endif  // OS_TRANSPARENT_MANAGEMENT_H
