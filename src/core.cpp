#include "core.h"
#include <algorithm>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
namespace wmpe {
Metric pages(std::uint64_t count, std::uint64_t size) {
  if (size == 0 || count > std::numeric_limits<std::uint64_t>::max() / size)
    return {};
  return count * size;
}
Metric headroom(Metric total, Metric limit) {
  if (!total || !limit || *total > *limit)
    return {};
  return *limit - *total;
}
std::optional<double> percent(Metric value, Metric total) {
  if (!value || !total || *total == 0 || *value > *total)
    return {};
  return static_cast<double>(*value) / static_cast<double>(*total) * 100.0;
}
Metric get(const Snapshot &s, const std::string &key) {
  const auto it = s.system.find(key);
  return it == s.system.end() ? Metric{} : it->second;
}
Options parse(const std::vector<std::string> &args) {
  Options o;
  bool command = false, top = false;
  for (std::size_t i = 0; i < args.size(); ++i) {
    const auto &a = args[i];
    if (a == "--json" && !o.json)
      o.json = true;
    else if (a == "--help" && !o.help)
      o.help = true;
    else if (a == "--version" && !o.version)
      o.version = true;
    else if (a == "--top" && !top) {
      top = true;
      if (++i == args.size())
        throw std::invalid_argument("--top requires 1..100");
      const auto &n = args[i];
      std::size_t v = 0;
      if (n.empty() || n.size() > 3)
        throw std::invalid_argument("--top requires 1..100");
      for (char c : n) {
        if (c < '0' || c > '9')
          throw std::invalid_argument("--top requires 1..100");
        v = v * 10 + static_cast<std::size_t>(c - '0');
      }
      if (v == 0 || v > 100)
        throw std::invalid_argument("--top requires 1..100");
      o.top = v;
    } else if (!command &&
               (a == "summary" || a == "processes" || a == "explain")) {
      o.command = a;
      command = true;
    } else
      throw std::invalid_argument("unknown or duplicate argument: " + a);
  }
  if ((o.help || o.version) && (args.size() != 1))
    throw std::invalid_argument("--help and --version must be used alone");
  return o;
}
void bound(std::vector<Process> &p, std::size_t top) {
  std::sort(p.begin(), p.end(), [](const Process &a, const Process &b) {
    if (a.working_set != b.working_set)
      return a.working_set > b.working_set;
    if (a.pid != b.pid)
      return a.pid < b.pid;
    return a.name < b.name;
  });
  if (p.size() > top)
    p.resize(top);
}
std::vector<Finding> findings(const Snapshot &s) {
  std::vector<Finding> f;
  const auto c =
      percent(get(s, "commit_total_bytes"), get(s, "commit_limit_bytes"));
  const auto available = get(s, "physical_available_bytes"),
             total = get(s, "physical_total_bytes");
  const auto physical = percent(headroom(available, total), total);
  if (c && *c >= 90)
    f.push_back(
        {"commit_near_limit",
         "Heuristic: commit utilization is at least 90%; current commit "
         "headroom is limited. The limit may grow with page files."});
  if (physical && *physical >= 90 && c && *c < 75)
    f.push_back({"physical_high_commit_headroom",
                 "Heuristic: physical utilization is at least 90% while commit "
                 "utilization is below 75%. High physical use alone does not "
                 "establish harmful pressure."});
  const auto pool = percent(get(s, "nonpaged_pool_bytes"), total);
  if (pool && *pool >= 10)
    f.push_back({"nonpaged_pool_large",
                 "Heuristic: nonpaged pool is at least 10% of physical memory; "
                 "investigate over time, without assuming a leak."});
  if (!s.processes.empty() && s.processes.front().working_set)
    f.push_back({"largest_observed_working_set",
                 "Process view ranks observed resident working sets; shared "
                 "pages can be counted in multiple processes. Private usage is "
                 "a separate commit measure."});
  f.push_back({"snapshot_limits",
               "A non-atomic snapshot cannot establish a leak or measure "
               "sustained pressure. Available memory includes reusable standby "
               "memory; useful caching is normal."});
  return f;
}
std::string quote(const std::string &s) {
  std::ostringstream out;
  out << '"';
  for (unsigned char c : s) {
    switch (c) {
    case '"':
      out << "\\\"";
      break;
    case '\\':
      out << "\\\\";
      break;
    default:
      if (c < 32)
        out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
            << static_cast<unsigned>(c) << std::dec;
      else
        out << static_cast<char>(c);
    }
  }
  out << '"';
  return out.str();
}
static void metric(std::ostream &out, Metric m) {
  if (m)
    out << *m;
  else
    out << "null";
}
std::string json(const Snapshot &s, const Options &o) {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << "{\"schema_version\":1,\"application_version\":\"1.0.0\",\"command\":"
      << quote(o.command) << ",\"status\":"
      << quote(!s.system_ok         ? "failure"
               : s.warnings.empty() ? "success"
                                    : "partial")
      << ",\"system\":{";
  bool first = true;
  for (const auto &kv : s.system) {
    if (!first)
      out << ',';
    first = false;
    out << quote(kv.first) << ':';
    metric(out, kv.second);
  }
  for (const auto &kv :
       std::vector<std::pair<std::string, std::optional<double>>>{
           {"physical_utilization_percent",
            percent(headroom(get(s, "physical_available_bytes"),
                             get(s, "physical_total_bytes")),
                    get(s, "physical_total_bytes"))},
           {"commit_utilization_percent",
            percent(get(s, "commit_total_bytes"),
                    get(s, "commit_limit_bytes"))}}) {
    if (!first)
      out << ',';
    first = false;
    out << quote(kv.first) << ':';
    if (kv.second)
      out << std::fixed << std::setprecision(6) << *kv.second;
    else
      out << "null";
  }
  out << "},\"processes_requested\":"
      << (s.processes_requested ? "true" : "false")
      << ",\"processes_enumerated\":" << s.enumerated << ",\"processes\":[";
  first = true;
  for (const auto &p : s.processes) {
    if (!first)
      out << ',';
    first = false;
    out << "{\"pid\":" << p.pid
        << ",\"name\":" << (p.name.empty() ? "null" : quote(p.name))
        << ",\"working_set_bytes\":";
    metric(out, p.working_set);
    out << ",\"private_usage_bytes\":";
    metric(out, p.private_usage);
    out << ",\"peak_working_set_bytes\":";
    metric(out, p.peak_working_set);
    out << ",\"win32_error\":" << p.error << '}';
  }
  out << "],\"warnings\":[";
  first = true;
  for (const auto &w : s.warnings) {
    if (!first)
      out << ',';
    first = false;
    out << "{\"code\":" << quote(w.code) << ",\"win32_error\":" << w.error
        << ",\"count\":" << w.count << '}';
  }
  out << "],\"findings\":[";
  first = true;
  for (const auto &f : findings(s)) {
    if (!first)
      out << ',';
    first = false;
    out << "{\"code\":" << quote(f.code) << ",\"message\":" << quote(f.message)
        << '}';
  }
  out << "]}\n";
  return out.str();
}
std::string human(const Snapshot &s, const Options &o) {
  std::ostringstream out;
  out << "windows-memory-pressure-explainer 1.0.0\n";
  if (o.command != "processes") {
    for (const auto &kv : s.system) {
      out << kv.first << ": ";
      if (kv.second)
        out << *kv.second;
      else
        out << "unavailable";
      out << '\n';
    }
    const auto c =
        percent(get(s, "commit_total_bytes"), get(s, "commit_limit_bytes"));
    const auto p = percent(headroom(get(s, "physical_available_bytes"),
                                    get(s, "physical_total_bytes")),
                           get(s, "physical_total_bytes"));
    out << "commit_utilization_percent: "
        << (c ? std::to_string(*c) : "unavailable")
        << "\nphysical_utilization_percent: "
        << (p ? std::to_string(*p) : "unavailable") << '\n';
  }
  if (s.processes_requested) {
    out << "PID | Name | Working set bytes | Private usage bytes | Peak "
           "working set bytes\n";
    for (const auto &p : s.processes) {
      out << p.pid << " | " << quote(p.name) << " | ";
      metric(out, p.working_set);
      out << " | ";
      metric(out, p.private_usage);
      out << " | ";
      metric(out, p.peak_working_set);
      out << '\n';
    }
  }
  if (o.command == "explain")
    for (const auto &f : findings(s))
      out << f.code << ": " << f.message << '\n';
  for (const auto &w : s.warnings)
    out << "warning: " << w.code << " (Win32 " << w.error << ", count "
        << w.count << ")\n";
  return out.str();
}
} // namespace wmpe
