#ifndef GHB_STRIDE_FDP_H
#define GHB_STRIDE_FDP_H

#include "modules.h"
#include <vector>
#include <unordered_map>
#include <list>

class ghb_stride_fdp : public champsim::modules::prefetcher {
public:
  using prefetcher::prefetcher;

  void prefetcher_initialise();
  void prefetcher_final_stats();
  uint32_t prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type, uint32_t metadata_in);
  uint32_t prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in);

private:
  struct GHBEntry {
    champsim::address addr;
    int prev_ptr;
  };

  unsigned int it_size = 2048;
  unsigned int ghb_size = 2048 * 16;
  unsigned int lookahead = 0;
  unsigned int degree = 4;
  unsigned int prefetch_distance = 16;
  unsigned int sequence_length = 3;

  unsigned int dynamic_counter = 3;
  unsigned int useful_prefetches = 0;
  unsigned int total_prefetches = 0;
  unsigned int late_prefetches = 0;

  unsigned int sampling_interval = 1000; // Default sampling interval
  unsigned int l2_access_counter = 0;    // Counter for L2 accesses

  unsigned int eviction_count = 0; // Counter for evicted blocks
  unsigned int useful_prefetches_during_interval = 0; // Interval-specific useful prefetches
  unsigned int late_prefetches_during_interval = 0;   // Interval-specific late prefetches
  unsigned int total_prefetches_during_interval = 0;   // Interval-specific total prefetches
  static constexpr unsigned int Tinterval = 1000; // Default interval threshold

  std::vector<GHBEntry> ghb = std::vector<GHBEntry>(ghb_size);

  // LRU data structure
  std::unordered_map<std::size_t, int> index_table; // Maps PC to GHB index
  std::list<std::size_t> lru_list; // Maintains LRU order
  int ghb_head = -1;

  void operate(champsim::address addr, champsim::address pc, uint32_t metadata_in);
  void issue_prefetch(champsim::address addr, uint32_t metadata_in);
  void adjust_aggressiveness();
  void update_counters();
  void reset_counters();
};

#endif // GHB_STRIDE_FDP_H
