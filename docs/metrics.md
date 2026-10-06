# Metrics and JSON contract

> Release preparation update (2026-10-05): see [current executable qualification and package provenance](../RELEASE-QUALIFICATION.md). Historical validation and pending-test statements in this document describe the original development snapshot. The final ZIP is assembled after qualification and has not been run on Windows.


Schema version 1; application version 1.0.0. Root fields: `schema_version`, `application_version`, `command`, `status` (success/partial/failure), `system`, `processes_requested`, `processes_enumerated`, `processes`, `warnings`, `findings`. No timestamps or random identifiers. Fixture output is deterministic. Live observations naturally vary. Consumers must use 64-bit integers (JavaScript consumers should preserve integers beyond 2^53).

| System field | Source/meaning |
|---|---|
| physical_total_bytes | GetPerformanceInfo PhysicalTotal times PageSize |
| physical_available_bytes | PhysicalAvailable times PageSize; standby/free/zero lists |
| physical_utilization_percent | (total-available)/total times 100 |
| commit_total_bytes | CommitTotal times PageSize |
| commit_limit_bytes | CommitLimit times PageSize; current, possibly extendable limit |
| commit_headroom_bytes | CommitLimit minus CommitTotal, after byte conversion |
| commit_utilization_percent | total/limit times 100 |
| commit_peak_bytes | CommitPeak times PageSize; since boot |
| system_cache_bytes | SystemCache times PageSize; standby plus system working set |
| paged_pool_bytes | KernelPaged times PageSize |
| nonpaged_pool_bytes | KernelNonpaged times PageSize |
| page_size_bytes | PageSize |
| process_count | ProcessCount from system query; can differ from enumeration |
| compression_bytes | null: no reliable documented compression observation implemented |
| shared_resident_bytes | null: no shared resident accounting implemented |
| hard_faults_per_second | null: no interval rate source implemented |
| page_file_occupied_bytes | null: actual disk page-file occupancy not measured |

If GetPerformanceInfo fails, GlobalMemoryStatusEx supplies only physical total and available; commit and pool values remain null, and a warning preserves the failure. In particular, its caller-relative page-file fields are not substituted for system commit. Page multiplication checks overflow; inconsistent headroom and percentages are null rather than wrapped, clamped, or guessed.

Process rows: `pid`, `name` (UTF-8 or null), `working_set_bytes`, `private_usage_bytes`, `peak_working_set_bytes`, `win32_error`. The byte fields come from GetProcessMemoryInfo PROCESS_MEMORY_COUNTERS_EX, respectively WorkingSetSize, PrivateUsage, PeakWorkingSetSize. Null means unavailable; zero means observed zero. Sorting: available working set before unavailable, descending working set, ascending PID, then name. Ranking happens before top-N selection. Peak/private values do not determine ranking.

Warnings: objects with `code`, `win32_error`, `count`. Process memory failures are grouped by error; each selected row also retains its own error. Error 0 on non-API warnings is not an operating-system failure code. Findings: `code` and `message`. Stable codes are commit_near_limit, physical_high_commit_headroom, nonpaged_pool_large, largest_observed_working_set, snapshot_limits. The latter is always present. Lack of a heuristic finding is not a health guarantee.

API references:
- https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-performance_information
- https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-getperformanceinfo
- https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-getprocessmemoryinfo
- https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex
- https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-createtoolhelp32snapshot
