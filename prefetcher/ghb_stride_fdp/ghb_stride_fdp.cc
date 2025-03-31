#include "ghb_stride_fdp.h"

#include <cmath>
#include <algorithm>

void ghb_stride_fdp::prefetcher_initialise() {
  ghb.clear();
  ghb.resize(ghb_size);
  index_table.clear();
  lru_list.clear();
  ghb_head = -1;
  dynamic_counter = 3; // Reset to "Middle-of-the-Road" aggressiveness
  eviction_count = 0; // Reset eviction counter
   reset_counters();
}

void ghb_stride_fdp::prefetcher_final_stats() {
  // Optionally, print final feedback metrics
}

uint32_t ghb_stride_fdp::prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type, uint32_t metadata_in) {
  operate(addr, ip, metadata_in);
  if (useful_prefetch) {
    ++useful_prefetches_during_interval;
  }
  if (!cache_hit && metadata_in == 1) { // Late prefetch detected: miss in cache but still in flight
    ++late_prefetches_during_interval;
  }

  return metadata_in;
}

uint32_t ghb_stride_fdp::prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in) {
  if (prefetch) {
    // Mark the prefetch as no longer in flight
    metadata_in = 0;
  }

  // Increment eviction count and check if the interval ends
  if (evicted_addr.to<std::size_t>() != 0) { // Non-zero evicted address indicates a valid eviction
    ++eviction_count;
    if (eviction_count >= Tinterval) {
      update_counters();       // Update all counters based on Equation 1
      adjust_aggressiveness(); // Adjust aggressiveness after counters are updated
      eviction_count = 0;      // Reset eviction counter
    }
  }

  return metadata_in;
}

void ghb_stride_fdp::operate(champsim::address addr, champsim::address pc, uint32_t metadata_in) {
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

  // Debugging information
  // champsim::address next_addr{champsim::block_number{cl_addr} + (lookahead + 1) * stride};
  // champsim::block_number next_block{cl_addr + stride};
  // int64_t stride_int = last_addresses[0].to<std::size_t>() - last_addresses[1].to<std::size_t>();
  // champsim::block_number next_block_int {cl_addr + stride_int};
  // printf("Address: %lx, address block start : %lx, 2 blocks from addr : %lx, PC : %lx, stride : %ld, stride_int : %ld , next_block : %lx, next_block_int : %lx\n",
  //        addr.to<std::size_t>(), 
  //        cl_addr.to<std::size_t>(),
  //        next_addr.to<std::size_t>(),
  //        pc.to<std::size_t>(),
  //        stride,
  //        stride_int,
  //        next_block.to<std::size_t>(),
  //        next_block_int.to<std::size_t>());

  if (consistent_stride) {
    // Issue prefetches
    for (unsigned int i = 1; i <= degree; ++i) {
      champsim::address pf_address{champsim::block_number{cl_addr} + (lookahead + i) * stride}; // Calculate full prefetch address
      if (champsim::offset(champsim::block_number{cl_addr},champsim::block_number{pf_address}) <= prefetch_distance) { // Check if the address is valid for prefetching
        issue_prefetch(pf_address, metadata_in);
        // printf("Prefetching address: %lx\n", pf_address.to<std::size_t>());
      }
      // issue_prefetch(pf_address, metadata_in);
    }
  }

  // Debug print GHB and index_table after updating
  // print_ghb();
  // print_index_table();
}

void ghb_stride_fdp::issue_prefetch(champsim::address pf_addr, uint32_t metadata_in) {
  bool fill_this_level = true;
  prefetch_line(pf_addr, fill_this_level, 1); // Set metadata_in to 1 to mark as in-flight
  ++total_prefetches_during_interval; // Increment interval-specific total prefetches
}

void ghb_stride_fdp::adjust_aggressiveness() {
  double accuracy = (total_prefetches > 0) ? static_cast<double>(useful_prefetches) / total_prefetches : 0.0;
  double lateness = (useful_prefetches > 0) ? static_cast<double>(late_prefetches) / useful_prefetches : 0.0;

  if (accuracy >= 0.75) { // High accuracy
    if (lateness > 0.01) {
      dynamic_counter = std::min(dynamic_counter + 1, 5u); // Increment
    }
  } else if (accuracy >= 0.40) { // Medium accuracy
    if (lateness > 0.01) {
      dynamic_counter = std::min(dynamic_counter + 1, 5u); // Increment
    }
  } else { // Low accuracy
    if (lateness > 0.01) {
      dynamic_counter = std::max(dynamic_counter - 1, 1u); // Decrement
    }
  }

  // Update aggressiveness based on the dynamic counter
  switch (dynamic_counter) {
    case 1: // Very Conservative
      prefetch_distance = 4;
      degree = 3;
      break;
    case 2: // Conservative
      prefetch_distance = 8;
      degree = 3;
      break;
    case 3: // Middle-of-the-Road
      prefetch_distance = 16;
      degree = 4;
      break;
    case 4: // Aggressive
      prefetch_distance = 32;
      degree = 5;
      break;
    case 5: // Very Aggressive
      prefetch_distance = 48;
      degree = 6;
      break;
  }
}

void ghb_stride_fdp::update_counters() {
  // Update counters based on Equation 1
  useful_prefetches = static_cast<unsigned int>(
    (1.0 / 2.0) * useful_prefetches +
    (1.0 / 2.0) * useful_prefetches_during_interval
  );

  late_prefetches = static_cast<unsigned int>(
    (1.0 / 2.0) * late_prefetches +
    (1.0 / 2.0) * late_prefetches_during_interval
  );

  total_prefetches = static_cast<unsigned int>(
    (1.0 / 2.0) * total_prefetches +
    (1.0 / 2.0) * total_prefetches_during_interval
  );

  // Reset interval-specific counters
  useful_prefetches_during_interval = 0;
  late_prefetches_during_interval = 0;
  total_prefetches_during_interval = 0;
}

void ghb_stride_fdp::reset_counters() {
  useful_prefetches = 0;
  total_prefetches = 0;
  late_prefetches = 0;
  eviction_count = 0;
  useful_prefetches_during_interval = 0;
  late_prefetches_during_interval = 0;
  total_prefetches_during_interval = 0;
}

void ghb_stride_fdp::print_ghb() const {
  printf("GHB Contents:\n");
  for (unsigned int i = 0; i < ghb_size; ++i) {
    if (ghb[i].addr.to<std::size_t>() != 0 || ghb[i].prev_ptr != -1) { // Print only valid entries
      printf("Index: %d, Addr: %lx, Prev Ptr: %d\n", i, ghb[i].addr.to<std::size_t>(), ghb[i].prev_ptr);
    }
  }
}

void ghb_stride_fdp::print_index_table() const {
  printf("Index Table Contents:\n");
  for (const auto& [key, value] : index_table) {
    printf("PC: %lx, GHB Index: %d\n", key, value);
  }
}
