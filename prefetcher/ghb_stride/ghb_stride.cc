#include "ghb_stride.h"

#include <vector>
#include <unordered_map>
#include <list>
#include <cmath>
#include <algorithm>

void ghb_stride::operate(champsim::address addr, champsim::address pc, uint32_t metadata_in) {
  champsim::block_number cl_addr{addr}; // Use block-aligned address
  std::size_t cl_pc = pc.to<std::size_t>(); // Use PC directly as is
  int ghb_index = (ghb_head + 1) % ghb_size;

  

  // Check if the PC is already in the index_table
  if (index_table.count(cl_pc)) {
    // Move the accessed PC to the front of the LRU list
    lru_list.remove(cl_pc);
    lru_list.push_front(cl_pc);
  } else {
    // If the table is full, evict the least recently used entry
    if (index_table.size() >= it_size) {
      std::size_t lru_pc = lru_list.back(); // Get the least recently used PC
      lru_list.pop_back();                 // Remove it from the LRU list
      index_table.erase(lru_pc);           // Remove it from the map
    }

    // Add the new PC to the front of the LRU list
    lru_list.push_front(cl_pc);
  }

  // Update GHB
  ghb[ghb_index] = {cl_addr, index_table.count(cl_pc) ? index_table[cl_pc] : -1};
  index_table[cl_pc] = ghb_index; // Store the GHB index in the map
  ghb_head = ghb_index;

  // Retrieve last `sequence_length` addresses
  std::vector<champsim::block_number> last_addresses;
  int ptr = ghb_index;
  while (ptr != -1 && last_addresses.size() < sequence_length) {
    last_addresses.push_back(ghb[ptr].addr);
    ptr = ghb[ptr].prev_ptr;
  }

  if (last_addresses.size() < sequence_length)
    return;

  // Compute strides and check for consistency
  bool consistent_stride = true;
  auto stride = champsim::offset(last_addresses[1], last_addresses[0]); // Reverse subtraction order
  for (size_t i = 1; i < last_addresses.size() - 1; ++i) {
    if (champsim::offset(last_addresses[i + 1], last_addresses[i]) != stride) { // Reverse subtraction order
      consistent_stride = false;
      break;
    }
  }
  // Dont touch this. This is for debugging purpose only.
  // champsim::address next_addr{champsim::block_number{cl_addr} + (lookahead + 5) * stride};
  // champsim::block_number next_block{cl_addr + 5*stride};
  // champsim::address next_block_addr{next_block};
  // int64_t stride_int = last_addresses[0].to<std::size_t>() - last_addresses[1].to<std::size_t>();
  // champsim::block_number next_block_int{cl_addr + stride_int};
  // auto distance = champsim::offset(champsim::block_number{cl_addr},champsim::block_number{next_addr});
  // printf("Address: %lx, address block start : %lx, 2 blocks from addr : %lx, PC : %lx, stride : %ld, stride_int : %ld , next_block : %lx, next_block_int : %lx, distance : %ld, distance_bool : %d, neext_block-addr: %lx\n"
  //               , addr.to<std::size_t>(), 
  //               cl_addr.to<std::size_t>(),
  //               next_addr.to<std::size_t>(),
  //               pc.to<std::size_t>(),
  //               stride,
  //               stride_int,
  //             next_block.to<std::size_t>(),
  //             next_block_int.to<std::size_t>(),
  //             distance,
  //             distance< 7,
  //             next_block_addr.to<std::size_t>()
  //               );
  
  if (consistent_stride) {
    // Issue prefetches
    for (unsigned int i = 1; i <= degree; ++i) {
      champsim::address pf_address{champsim::block_number{cl_addr} + (lookahead + i) * stride}; // Calculate full prefetch address
      // printf("Prefetching address: %lx\n", pf_address.to<std::size_t>());
      issue_prefetch(pf_address, metadata_in);
    }
  }

  // Debug print GHB and index_table after updating
  // print_ghb();
  // print_index_table();
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

void ghb_stride::print_ghb() const {
  printf("GHB Contents:\n");
  for (unsigned int i = 0; i < ghb_size; ++i) {
    if (ghb[i].addr.to<std::size_t>() != 0 || ghb[i].prev_ptr != -1) { // Print only valid entries
      printf("Index: %d, Addr: %lx, Prev Ptr: %d\n", i, ghb[i].addr.to<std::size_t>(), ghb[i].prev_ptr);
    }
  }
}

void ghb_stride::print_index_table() const {
  printf("Index Table Contents:\n");
  for (const auto& [key, value] : index_table) {
    printf("PC: %lx, GHB Index: %d\n", key, value);
  }
}