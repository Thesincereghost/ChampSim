#include "ghb_stride.h"

#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>

// constexpr int DEFAULT_IT_SIZE = 256;
// constexpr int DEFAULT_GHB_SIZE = 256;
// constexpr int DEFAULT_LOOKAHEAD = 4;
// constexpr int DEFAULT_PREFETCH_DEGREE = 4;
// constexpr int DEFAULT_SEQUENCE_LENGTH = 3; // Parametrize the sequence length

void ghb_stride::operate(champsim::address addr, champsim::address pc, uint32_t metadata_in) {
  unsigned int it_index = static_cast<unsigned int>(pc.to<std::size_t>()) % it_size; // Explicitly cast pc to unsigned int
  int ghb_index = (ghb_head + 1) % ghb_size;

  // Update GHB
  ghb[ghb_index] = {addr, index_table[it_index]};
  index_table[it_index] = ghb_index;
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
  index_table.clear();
  ghb_head = -1;
}

void ghb_stride::prefetcher_final_stats() {
  // Print final statistics (if any)
  // Example: std::cout << "ghb_stride: Final stats..." << std::endl;
}

uint32_t ghb_stride::prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type,
                                              uint32_t metadata_in) {
//   if (type == access_type::LOAD) {
    operate(addr, ip, metadata_in); // Pass metadata_in to operate
//   }
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

// void ghb_stride::bind(CACHE* cache) {
//   // This method is required for integration with the CACHE class.
//   // Currently, no specific binding logic is needed for ghb_stride.
//   (void)cache; // Suppress unused parameter warning
// }