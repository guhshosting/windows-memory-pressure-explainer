#include "core.h"
#include <iostream>
#include <limits>
#include <stdexcept>
static int failures = 0;
static void check(bool ok, const char *name) {
  if (!ok) {
    std::cerr << name << '\n';
    ++failures;
  }
}
int main() try {
  using namespace wmpe;
  check(pages(2, 4096) == 8192, "pages");
  check(pages(0, 4096) == 0, "zero pages");
  check(!pages(1, 0), "zero page size");
  check(!pages(std::numeric_limits<std::uint64_t>::max(), 2), "overflow");
  check(pages(std::numeric_limits<std::uint64_t>::max(), 1) ==
            std::numeric_limits<std::uint64_t>::max(),
        "extreme");
  check(headroom(20, 100) == 80, "headroom");
  check(headroom(100, 100) == 0, "zero headroom");
  check(!headroom(101, 100), "inconsistent");
  check(!headroom({}, 100), "unavailable");
  check(percent(1, 4) == 25.0, "percent");
  check(!percent(0, 0), "zero denominator");
  check(!percent(2, 1), "invalid percent");
  check(percent(0, 1) == 0.0, "zero percent");
  std::vector<Process> p{{9, "z", 10, 999, 20, 0},
                         {2, "a", 10, 1, 30, 0},
                         {1, "b", {}, 22, {}, 5}};
  bound(p, 2);
  check(p.size() == 2 && p[0].pid == 2 && p[1].pid == 9, "sorting and top");
  check(p[0].private_usage == 1, "private distinct");
  bound(p, 0);
  check(p.empty(), "zero bound");
  check(parse({}).top == 10, "defaults");
  check(parse({"processes", "--top", "100", "--json"}).top == 100, "max");
  for (const auto &a :
       std::vector<std::vector<std::string>>{{"--top", "0"},
                                             {"--top", "101"},
                                             {"--top", "-1"},
                                             {"--top", "1x"},
                                             {"--top"},
                                             {"bad"},
                                             {"summary", "processes"},
                                             {"--json", "--json"},
                                             {"--help", "--json"}}) {
    bool threw = false;
    try {
      static_cast<void>(parse(a));
    } catch (const std::invalid_argument &) {
      threw = true;
    }
    check(threw, "invalid parsing");
  }
  Snapshot s;
  s.system_ok = true;
  s.system = {{"physical_total_bytes", 100},
              {"physical_available_bytes", 5},
              {"commit_total_bytes", 50},
              {"commit_limit_bytes", 100},
              {"nonpaged_pool_bytes", 10}};
  auto f = findings(s);
  check(f.size() == 3 && f[0].code == "physical_high_commit_headroom" &&
            f[1].code == "nonpaged_pool_large",
        "findings");
  s.system["commit_total_bytes"] = 90;
  check(findings(s)[0].code == "commit_near_limit", "commit threshold");
  s.system["commit_total_bytes"] = {};
  check(findings(s).size() == 2, "null findings");
  s.processes = {{1, "a\"\\\n", 12, 34, 56, 0}};
  s.system["compression_bytes"] = {};
  const auto j = json(s, Options{});
  check(j == json(s, Options{}), "determinism");
  check(j.find("\"working_set_bytes\":12,\"private_usage_bytes\":34") !=
            std::string::npos,
        "json distinct");
  check(j.find("\"compression_bytes\":null") != std::string::npos, "json null");
  check(quote("\n\"\\") == "\"\\u000a\\\"\\\\\"", "escaping");
  check(j.find("\"schema_version\":1") != std::string::npos, "schema version");
  if (failures == 0)
    std::cout << "All unit checks passed\n";
  return failures == 0 ? 0 : 1;
} catch (...) {
  return 1;
}
