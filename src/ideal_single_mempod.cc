#include "ideal_single_mempod.h"

#if (MEMORY_USE_OS_TRANSPARENT_MANAGEMENT == ENABLE)
#if (IDEAL_SINGLE_MEMPOD == ENABLE)

// complete
OS_TRANSPARENT_MANAGEMENT::OS_TRANSPARENT_MANAGEMENT(uint64_t max_address, uint64_t fast_memory_max_address)
    : total_capacity(max_address), fast_memory_capacity(fast_memory_max_address),
    total_capacity_at_granularity(max_address >> DATA_MANAGEMENT_OFFSET_BITS),
    fast_memory_capacity_at_granularity(fast_memory_max_address >> DATA_MANAGEMENT_OFFSET_BITS),
    fast_memory_offset_bit(DATA_MANAGEMENT_OFFSET_BITS),
    mea_counter_table(*(new std::unordered_map<REMAPPING_TABLE_ENTRY_WIDTH, mea_counter_entry>())),
    address_remapping_table(*(new std::unordered_map<REMAPPING_TABLE_ENTRY_WIDTH, REMAPPING_TABLE_ENTRY_WIDTH>())),
    invert_address_remapping_table(*(new std::unordered_map<REMAPPING_TABLE_ENTRY_WIDTH, REMAPPING_TABLE_ENTRY_WIDTH>()))
{
    remapping_request_queue_congestion = 0;
#if (PRINT_SWAP_DETAIL)
    swap_request = 0;
#endif // PRINT_SWAP_DETAIL

    /* initializing address_remapping_table and invert_address_remapping_table */
    // TODO: I think there is faster way to construct mapping.

    for (REMAPPING_TABLE_ENTRY_WIDTH itr_map = 0; itr_map < fast_memory_capacity_at_granularity; itr_map++)
    {
        address_remapping_table[itr_map] = itr_map;
        invert_address_remapping_table[itr_map] = itr_map;
    }
    for (REMAPPING_TABLE_ENTRY_WIDTH itr_map = fast_memory_capacity_at_granularity; itr_map < total_capacity_at_granularity; itr_map++)
    {
        address_remapping_table[itr_map] = itr_map;
    }
    assert(address_remapping_table.size() == total_capacity_at_granularity);
    assert(invert_address_remapping_table.size() == fast_memory_capacity_at_granularity);
};

// complete
OS_TRANSPARENT_MANAGEMENT::~OS_TRANSPARENT_MANAGEMENT()
{
    outputchampsimstatistics.remapping_request_queue_congestion = remapping_request_queue_congestion;

#if (PRINT_SWAP_DETAIL)
    outputchampsimstatistics.swap_request = swap_request;
#endif // PRINT_SWAP_DETAIL

    delete& mea_counter_table;
    delete& address_remapping_table;
    delete& invert_address_remapping_table;
};

// incomplete
bool OS_TRANSPARENT_MANAGEMENT::memory_activity_tracking(uint64_t address, uint8_t type, float queue_busy_degree)
{

#if (TRACKING_LOAD_ONLY)
    if (type_origin == RFO || type_origin == WRITEBACK) // CPU Store Instruction and LLC Writeback is ignored
    {
        return true;
    }
#endif // TRACKING_LOAD_ONLY

#if (TRACKING_READ_ONLY)
    if (type == 2) // Memory Write is ignored
    {
        return true;
    }
#endif // TRACKING_READ_ONLY

    if (address >= total_capacity)
    {
        std::cout << __func__ << ": address input error." << std::endl;
        return false;
    }

    uint64_t data_block_address = address >> DATA_MANAGEMENT_OFFSET_BITS;   // calculate the data block address
    uint64_t line_location_table_index = data_block_address % fast_memory_capacity_at_data_block_granularity;   // calculate the index in line location table
    REMAPPING_LOCATION_WIDTH location = static_cast<REMAPPING_LOCATION_WIDTH>(data_block_address / fast_memory_capacity_at_data_block_granularity); // calculate the location in the entry of the line location table

    if (location >= REMAPPING_LOCATION_WIDTH(RemappingLocation::Max))
    {
        std::cout << __func__ << ": address input error (location)." << std::endl;
        abort();
    }

    uint8_t msb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - REMAPPING_LOCATION_WIDTH_BITS * location;
    uint8_t lsb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - (REMAPPING_LOCATION_WIDTH_BITS + REMAPPING_LOCATION_WIDTH_BITS * location - 1);

    REMAPPING_LOCATION_WIDTH remapping_location = get_bits(line_location_table.at(line_location_table_index), msb_in_location_table_entry, lsb_in_location_table_entry);

    if (type == 1)  // for read request
    {
        if (counter_table.at(data_block_address) < COUNTER_MAX_VALUE)
        {
            counter_table[data_block_address]++;    // increment its counter
        }

        if (counter_table.at(data_block_address) >= hotness_threshold)
        {
            hotness_table.at(data_block_address) = true;    // mark hot data block
        }

    }
    else if (type == 2) // for write request
    {
        if (counter_table.at(data_block_address) < COUNTER_MAX_VALUE)
        {
            counter_table[data_block_address]++;    // increment its counter
        }

        if (counter_table.at(data_block_address) >= hotness_threshold)
        {
            hotness_table.at(data_block_address) = true;    // mark hot data block
        }
    }
    else
    {
        std::cout << __func__ << ": type input error." << std::endl;
        assert(0);
    }

    // add new remapping requests to queue
    if ((hotness_table.at(data_block_address) == true) && (remapping_location != REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero)))
    {
        RemappingRequest remapping_request;
        REMAPPING_LOCATION_WIDTH fm_location = REMAPPING_LOCATION_WIDTH(RemappingLocation::Max);

        uint8_t fm_msb_in_location_table_entry;
        uint8_t fm_lsb_in_location_table_entry;
        REMAPPING_LOCATION_WIDTH fm_remapping_location;

        // find the fm_location in the entry of line_location_table (where RemappingLocation::Zero is in the entry of line_location_table)
        for (REMAPPING_LOCATION_WIDTH i = REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero); i < REMAPPING_LOCATION_WIDTH(RemappingLocation::Max); i++)
        {
            fm_msb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - REMAPPING_LOCATION_WIDTH_BITS * i;
            fm_lsb_in_location_table_entry = LOCATION_TABLE_ENTRY_MSB - (REMAPPING_LOCATION_WIDTH_BITS + REMAPPING_LOCATION_WIDTH_BITS * i - 1);

            fm_remapping_location = get_bits(line_location_table.at(line_location_table_index), fm_msb_in_location_table_entry, fm_lsb_in_location_table_entry);

            if (fm_remapping_location == REMAPPING_LOCATION_WIDTH(RemappingLocation::Zero))
            {
                // found the location of fm_remapping_location in the entry of line_location_table
                fm_location = i;
                break;
            }
        }

        if (fm_location == REMAPPING_LOCATION_WIDTH(RemappingLocation::Max))
        {
            std::cout << __func__ << ": find the fm_location error." << std::endl;
            abort();
        }

        if (fm_remapping_location == remapping_location)    // check
        {
            std::cout << __func__ << ": add new remapping request error 1." << std::endl;
            printf("line_location_table.at(%ld): %d.\n", line_location_table_index, line_location_table.at(line_location_table_index));
            printf("remapping_location: %d, fm_remapping_location: %d.\n", remapping_location, fm_remapping_location);
            printf("fm_msb_in_location_table_entry: %d, fm_lsb_in_location_table_entry: %d.\n", fm_msb_in_location_table_entry, fm_lsb_in_location_table_entry);
            printf("msb_in_location_table_entry: %d, lsb_in_location_table_entry: %d.\n", msb_in_location_table_entry, lsb_in_location_table_entry);
            abort();
        }

        remapping_request.address_in_fm = replace_bits(line_location_table_index << DATA_MANAGEMENT_OFFSET_BITS, uint64_t(fm_remapping_location) << fast_memory_offset_bit, congruence_group_msb, fast_memory_offset_bit);
        remapping_request.address_in_sm = replace_bits(line_location_table_index << DATA_MANAGEMENT_OFFSET_BITS, uint64_t(remapping_location) << fast_memory_offset_bit, congruence_group_msb, fast_memory_offset_bit);

        // indicate the positions in line location table entry for address_in_fm and address_in_sm.
        remapping_request.fm_location = fm_location;
        remapping_request.sm_location = location;

        remapping_request.size = DATA_GRANULARITY_IN_CACHE_LINE;

        if (queue_busy_degree <= QUEUE_BUSY_DEGREE_THRESHOLD)
        {
            enqueue_remapping_request(remapping_request);
        }
    }

    return true;
};

// complete
void OS_TRANSPARENT_MANAGEMENT::physical_to_hardware_address(PACKET& packet)
{
    uint64_t data_segment_address = packet.address >> DATA_MANAGEMENT_OFFSET_BITS;
    uint64_t data_segment_offset = packet.address - data_segment_address;
    packet.h_address = address_remapping_table[data_segment_address] << DATA_MANAGEMENT_OFFSET_BITS + data_segment_offset;
};

// complete
void OS_TRANSPARENT_MANAGEMENT::physical_to_hardware_address(uint64_t& address)
{
    uint64_t data_segment_address = address >> DATA_MANAGEMENT_OFFSET_BITS;
    uint64_t data_segment_offset = address - data_segment_address;
    address = address_remapping_table[data_segment_address] << DATA_MANAGEMENT_OFFSET_BITS + data_segment_offset;
};

// complete
bool OS_TRANSPARENT_MANAGEMENT::issue_remapping_request(RemappingRequest& remapping_request)
{
    if (remapping_request_queue.empty() == false)
    {
        remapping_request = remapping_request_queue.front();
        return true;
    }

    return false;
};

// complete
bool OS_TRANSPARENT_MANAGEMENT::finish_remapping_request()
{
    if (remapping_request_queue.empty() == false)
    {   
        RemappingRequest remapping_request = remapping_request_queue.front();
        remapping_request_queue.pop_front();

        /* update address_remapping_table */
        uint64_t data_segment_p_address_fm = remapping_request.p_address_in_fm >> DATA_MANAGEMENT_OFFSET_BITS;
        uint64_t data_segment_p_address_sm = remapping_request.p_address_in_sm >> DATA_MANAGEMENT_OFFSET_BITS;

        uint64_t data_segment_h_address_fm = remapping_request.h_address_in_fm >> DATA_MANAGEMENT_OFFSET_BITS;
        uint64_t data_segment_h_address_sm = remapping_request.h_address_in_sm >> DATA_MANAGEMENT_OFFSET_BITS;

        // sanity check
        if (data_segment_h_address_fm == data_segment_h_address_sm)
        {
            std::cout << __func__ << ": read remapping location error." << std::endl;
            abort();
        }

        address_remapping_table[data_segment_p_address_sm] = data_segment_h_address_fm;
        address_remapping_table[data_segment_p_address_fm] = data_segment_h_address_sm;

        /* update invert_address_remapping_table*/
        invert_address_remapping_table[data_segment_h_address_fm] = data_segment_p_address_sm;
    }
    else
    {
        std::cout << __func__ << ": remapping error." << std::endl;
        assert(0);
        return false;   // error
    }
    return true;
};

// complete
void OS_TRANSPARENT_MANAGEMENT::cold_data_detection()
{
    cycle++;
}

// incomplete
bool OS_TRANSPARENT_MANAGEMENT::enqueue_remapping_request(RemappingRequest& remapping_request)
{
    uint64_t data_segment_address = remapping_request.h_address_in_sm >> DATA_MANAGEMENT_OFFSET_BITS;
    uint64_t line_location_table_index = data_block_address % fast_memory_capacity_at_data_block_granularity;

    // check duplicated remapping request in remapping_request_queue
    // if duplicated remapping requests exist, we won't add this new remapping request into the remapping_request_queue.
    bool duplicated_remapping_request = false;
    for (uint64_t i = 0; i < remapping_request_queue.size(); i++)
    {
        uint64_t data_block_address_to_check = remapping_request_queue[i].address_in_fm >> DATA_MANAGEMENT_OFFSET_BITS;
        uint64_t line_location_table_index_to_check = data_block_address_to_check % fast_memory_capacity_at_data_block_granularity;

        if (line_location_table_index_to_check == line_location_table_index)
        {
            duplicated_remapping_request = true;    // find a duplicated remapping request

            break;
        }
    }

    if (duplicated_remapping_request == false)
    {
        if (remapping_request_queue.size() < REMAPPING_REQUEST_QUEUE_LENGTH)
        {
            if (remapping_request.address_in_fm == remapping_request.address_in_sm)    // check
            {
                std::cout << __func__ << ": add new remapping request error 2." << std::endl;
                abort();
            }

            // enqueue a remapping request
            remapping_request_queue.push_back(remapping_request);
        }
        else
        {
            //std::cout << __func__ << ": remapping_request_queue is full." << std::endl;
            remapping_request_queue_congestion++;
        }
    }
    else
    {
        return false;
    }

    // new remapping request is issued.
#if (PRINT_SWAP_DETAIL)
    swap_request++;
#endif
    return true;
}

#endif  // IDEAL_SINGLE_MEMPOD
#endif  // MEMORY_USE_OS_TRANSPARENT_MANAGEMENT
