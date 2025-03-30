#ifndef GHB_STRIDE_H
#define GHB_STRIDE_H

// #include "cache.h"
#include "modules.h"
#include <vector>
#include <unordered_map>
#include <list>

class ghb_stride : public champsim::modules::prefetcher { 

public:
  using prefetcher::prefetcher;
  static constexpr unsigned int DEFAULT_IT_SIZE = 2048;
  static constexpr unsigned int DEFAULT_GHB_SIZE = 2048*4;
  static constexpr unsigned int DEFAULT_LOOKAHEAD = 0;
  static constexpr unsigned int DEFAULT_PREFETCH_DEGREE = 4;
  static constexpr unsigned int DEFAULT_SEQUENCE_LENGTH = 3;

  void prefetcher_initialise();
  void prefetcher_final_stats();
  uint32_t prefetcher_cache_operate(champsim::address addr, champsim::address ip, uint8_t cache_hit, bool useful_prefetch, access_type type, uint32_t metadata_in);
  uint32_t prefetcher_cache_fill(champsim::address addr, long set, long way, uint8_t prefetch, champsim::address evicted_addr, uint32_t metadata_in);

//   void bind(CACHE* cache); // Add bind method for integration

private:
  struct GHBEntry {
    champsim::address addr;
    int prev_ptr;
  };

  void operate(champsim::address addr, champsim::address pc, uint32_t metadata_in);
  void issue_prefetch(champsim::address addr, uint32_t metadata_in);

  unsigned int it_size = DEFAULT_IT_SIZE;
  unsigned int ghb_size = DEFAULT_GHB_SIZE;
  unsigned int lookahead = DEFAULT_LOOKAHEAD;
  unsigned int degree = DEFAULT_PREFETCH_DEGREE;
  unsigned int sequence_length = DEFAULT_SEQUENCE_LENGTH;
  std::vector<GHBEntry> ghb = std::vector<GHBEntry>(DEFAULT_GHB_SIZE);

  // LRU data structure
  std::unordered_map<std::size_t, int> index_table; // Maps PC to GHB index
  std::list<std::size_t> lru_list; // Maintains LRU order
  int ghb_head = -1;
};

#endif // GHB_STRIDE_H
