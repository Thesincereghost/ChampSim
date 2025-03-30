#include "ghb_stride.h"

#include <vector>
#include <unordered_map>
#include <list>
#include <cmath>
#include <algorithm>

void ghb_stride::operate(champsim::address addr, champsim::address pc, uint32_t metadata_in) {
  unsigned int it_index = pc.to<std::size_t>(); // Use PC directly as the key
  int ghb_index = (ghb_head + 1) % ghb_size;

  // Check if the PC is already in the index_table
  if (index_table.count(it_index)) {
    // Move the accessed PC to the front of the LRU list
    lru_list.remove(it_index);
    lru_list.push_front(it_index);
  } else {
    // If the table is full, evict the least recently used entry
    if (index_table.size() >= it_size) {
      int lru_pc = lru_list.back(); // Get the least recently used PC
      lru_list.pop_back(); // Remove it from the LRU list
      index_table.erase(lru_pc); // Remove it from the map
    }

    // Add the new PC to the front of the LRU list
    lru_list.push_front(it_index);
  }

  // Update GHB
  ghb[ghb_index] = {addr, index_table.count(it_index) ? index_table[it_index] : -1};
  index_table[it_index] = ghb_index; // Store the GHB index in the map
  ghb_head = ghb_index;

  // Retrieve last `sequence_length` addresses
  std::vector<champsim::address> last_addresses;
  int ptr = ghb_index;
  while (ptr != -1 && last_addresses.size() < sequence_length) { 
    last_addresses.push_back(ghb[ptr].addr);
    ptr = ghb[ptr].prev_ptr;
  }

  if (last_addresses.size() < sequence_length)
    return;

  // Compute strides and check for consistency
  bool consistent_stride = true;
  auto stride = last_addresses[0].to<std::size_t>() - last_addresses[1].to<std::size_t>(); // Convert to numeric type before subtraction
  for (size_t i = 1; i < last_addresses.size() - 1; ++i) {
    if (last_addresses[i].to<std::size_t>() - last_addresses[i + 1].to<std::size_t>() != stride) {
      consistent_stride = false;
      break;
    }
  }

  if (consistent_stride) {
    // Issue prefetches
    for (unsigned int i = 1; i <= degree; ++i) { // Use unsigned int for consistency
      champsim::address pf_addr = addr + (lookahead + i) * stride;
      issue_prefetch(pf_addr, metadata_in); // Updated to call renamed function
    }
  }
}

void ghb_stride::prefetcher_initialise() {
  // Initialize internal data structures
  ghb.clear();
  ghb.resize(ghb_size);
  index_table.clear(); // Clear the unordered_map
  lru_list.clear(); // Clear the LRU list
  ghb_head = -1;
}

void ghb_stride::prefetcher_final_stats() {
  // Print final statistics (if any)
  // Example: std::cout << "ghb_stride: Final stats..." << std::endl;
}

uint32_t ghb_stride::prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
                                              uint32_t metadata_in) {
  operate(addr, ip, metadata_in); // Pass metadata_in to operate
  return metadata_in;
}

uint32_t ghb_stride::prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in) {
  // Handle L2 fill operation (if needed)
  return metadata_in;
}

void ghb_stride::issue_prefetch(champsim::address pf_addr, uint32_t metadata_in) {
  // Issue a prefetch request using the simulator-provided function
  bool fill_this_level = true; // Set to true to fill this cache level (L2)
  prefetch_line(pf_addr, fill_this_level, metadata_in); // Call simulator-provided function
}