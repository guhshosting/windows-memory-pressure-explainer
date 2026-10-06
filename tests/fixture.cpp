#include "core.h"
#include <iostream>
int main() try {
  wmpe::Snapshot s;
  s.system_ok = true;
  s.processes_requested = true;
  s.enumerated = 3;
  s.system = {{"physical_total_bytes", 100}, {"physical_available_bytes", 0},
              {"commit_total_bytes", 90},    {"commit_limit_bytes", 100},
              {"commit_headroom_bytes", 10}, {"compression_bytes", {}},
              {"page_size_bytes", 4096}};
  s.processes = {{9, "z", 12, 999, 56, 0},
                 {2, "Unicode \xe9\x9b\xaa\"\\\n", 12, 34, 20, 0},
                 {1, "unavailable", {}, {}, {}, 5}};
  s.warnings = {{"process_memory_unavailable", 5, 1}};
  wmpe::bound(s.processes, 3);
  std::cout << wmpe::json(s, wmpe::Options{});
  return std::cout ? 0 : 1;
} catch (...) {
  return 1;
}
