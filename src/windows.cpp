#include "core.h"
#include <array>
#include <algorithm>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <windows.h>
namespace wmpe {
class Handle {
  HANDLE h_;

public:
  explicit Handle(HANDLE h) : h_(h) {}
  ~Handle() {
    if (h_ && h_ != INVALID_HANDLE_VALUE)
      CloseHandle(h_);
  }
  Handle(const Handle &) = delete;
  Handle &operator=(const Handle &) = delete;
  HANDLE get() const { return h_; }
  bool valid() const { return h_ && h_ != INVALID_HANDLE_VALUE; }
};
static std::string utf8(const wchar_t *text) {
  const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, -1,
                                       nullptr, 0, nullptr, nullptr);
  if (size <= 0 || size > 32768)
    return {};
  std::string result(static_cast<std::size_t>(size), '\0');
  if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, -1,
                          result.data(), size, nullptr, nullptr) != size)
    return {};
  result.pop_back();
  return result;
}
Snapshot acquire(bool processes) {
  Snapshot s;
  s.processes_requested = processes;
  for (const char *key :
       {"physical_total_bytes", "physical_available_bytes",
        "commit_total_bytes", "commit_limit_bytes", "commit_headroom_bytes",
        "commit_peak_bytes", "system_cache_bytes", "paged_pool_bytes",
        "nonpaged_pool_bytes", "process_count", "page_size_bytes",
        "hard_faults_per_second", "shared_resident_bytes", "compression_bytes",
        "page_file_occupied_bytes"})
    s.system.emplace(key, Metric{});
  PERFORMANCE_INFORMATION info{};
  info.cb = sizeof(info);
  if (GetPerformanceInfo(&info, sizeof(info))) {
    s.system_ok = true;
    s.system["physical_total_bytes"] = pages(info.PhysicalTotal, info.PageSize);
    s.system["physical_available_bytes"] =
        pages(info.PhysicalAvailable, info.PageSize);
    s.system["commit_total_bytes"] = pages(info.CommitTotal, info.PageSize);
    s.system["commit_limit_bytes"] = pages(info.CommitLimit, info.PageSize);
    s.system["commit_peak_bytes"] = pages(info.CommitPeak, info.PageSize);
    s.system["system_cache_bytes"] = pages(info.SystemCache, info.PageSize);
    s.system["paged_pool_bytes"] = pages(info.KernelPaged, info.PageSize);
    s.system["nonpaged_pool_bytes"] = pages(info.KernelNonpaged, info.PageSize);
    s.system["process_count"] = info.ProcessCount;
    s.system["page_size_bytes"] = info.PageSize;
    for (const auto &kv : s.system)
      if (kv.first != "commit_headroom_bytes" &&
          kv.first != "hard_faults_per_second" &&
          kv.first != "shared_resident_bytes" &&
          kv.first != "compression_bytes" &&
          kv.first != "page_file_occupied_bytes" && !kv.second)
        s.warnings.push_back({"page_conversion_overflow", 0});
  } else {
    s.warnings.push_back({"GetPerformanceInfo_failed", GetLastError()});
    MEMORYSTATUSEX m{};
    m.dwLength = sizeof(m);
    if (GlobalMemoryStatusEx(&m)) {
      s.system_ok = true;
      s.system["physical_total_bytes"] = m.ullTotalPhys;
      s.system["physical_available_bytes"] = m.ullAvailPhys;
    } else
      s.warnings.push_back({"GlobalMemoryStatusEx_failed", GetLastError()});
  }
  s.system["commit_headroom_bytes"] =
      headroom(get(s, "commit_total_bytes"), get(s, "commit_limit_bytes"));
  if (!processes)
    return s;
  Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
  if (!snapshot.valid()) {
    s.warnings.push_back({"process_enumeration_failed", GetLastError()});
    return s;
  }
  PROCESSENTRY32W entry{};
  entry.dwSize = sizeof(entry);
  if (!Process32FirstW(snapshot.get(), &entry)) {
    s.warnings.push_back({"Process32First_failed", GetLastError()});
    return s;
  }
  constexpr std::size_t maximum = 65536;
  do {
    if (s.processes.size() == maximum) {
      s.warnings.push_back({"process_enumeration_capped", 0});
      break;
    }
    Process p;
    p.pid = entry.th32ProcessID;
    p.name = utf8(entry.szExeFile);
    if (p.name.empty())
      s.warnings.push_back({"process_name_unavailable", GetLastError()});
    Handle process(
        OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, p.pid));
    if (process.valid()) {
      PROCESS_MEMORY_COUNTERS_EX m{};
      m.cb = sizeof(m);
      if (GetProcessMemoryInfo(process.get(),
                               reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&m),
                               sizeof(m))) {
        p.working_set = m.WorkingSetSize;
        p.private_usage = m.PrivateUsage;
        p.peak_working_set = m.PeakWorkingSetSize;
      } else
        p.error = GetLastError();
    } else
      p.error = GetLastError();
    if (p.error != 0) {
      const auto warning = std::find_if(
          s.warnings.begin(), s.warnings.end(), [&p](const Warning &w) {
            return w.code == "process_memory_unavailable" && w.error == p.error;
          });
      if (warning != s.warnings.end())
        ++warning->count;
      else
        s.warnings.push_back({"process_memory_unavailable", p.error});
    }
    s.processes.push_back(std::move(p));
  } while (Process32NextW(snapshot.get(), &entry));
  if (s.processes.size() < maximum && GetLastError() != ERROR_NO_MORE_FILES)
    s.warnings.push_back({"Process32Next_failed", GetLastError()});
  s.enumerated = s.processes.size();
  return s;
}
} // namespace wmpe
