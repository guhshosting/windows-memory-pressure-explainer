#ifndef WMPE_CORE_H
#define WMPE_CORE_H
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>
namespace wmpe {
using Metric = std::optional<std::uint64_t>;
struct Process {
  std::uint32_t pid{};
  std::string name;
  Metric working_set, private_usage, peak_working_set;
  std::uint32_t error{};
};
struct Warning {
  std::string code;
  std::uint32_t error{};
  std::size_t count = 1;
};
struct Snapshot {
  Snapshot() = default;
  Snapshot(const Snapshot &) = default;
  Snapshot &operator=(const Snapshot &) = default;
  std::map<std::string, Metric> system;
  std::vector<Process> processes;
  std::vector<Warning> warnings;
  bool system_ok{};
  bool processes_requested{};
  std::size_t enumerated{};
};
struct Finding {
  std::string code, message;
};
struct Options {
  std::string command = "explain";
  std::size_t top = 10;
  bool json{}, help{}, version{};
};
Metric pages(std::uint64_t count, std::uint64_t size);
Metric headroom(Metric total, Metric limit);
std::optional<double> percent(Metric value, Metric total);
Metric get(const Snapshot &s, const std::string &key);
Options parse(const std::vector<std::string> &args);
void bound(std::vector<Process> &p, std::size_t top);
std::vector<Finding> findings(const Snapshot &s);
std::string quote(const std::string &s);
std::string json(const Snapshot &s, const Options &o);
std::string human(const Snapshot &s, const Options &o);
Snapshot acquire(bool processes);
} // namespace wmpe

#endif

