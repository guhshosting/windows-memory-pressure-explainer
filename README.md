# windows-memory-pressure-explainer 1.0.0

> Release preparation update (2026-10-05): see [current executable qualification and package provenance](RELEASE-QUALIFICATION.md). Historical validation and pending-test statements in this document describe the original development snapshot. The final ZIP is assembled after qualification and has not been run on Windows.


A portable Windows x64 command-line utility that explains a bounded memory snapshot. C++17, MIT licensed, no installer or runtime redistributable required (static C++ runtime).

Run from a normal, unelevated terminal:

```text
windows-memory-pressure-explainer
windows-memory-pressure-explainer summary
windows-memory-pressure-explainer processes --top 20
windows-memory-pressure-explainer explain --json
windows-memory-pressure-explainer --help
windows-memory-pressure-explainer --version
```

Default: `explain`, top 10 processes by observed working set. `--top N` accepts 1 through 100, with summary accepting it but having no process view. Options may precede or follow a command. Duplicate/unknown options are rejected. Help/version must be used alone. Output is bytes, counts, or explicitly named percentages; JSON uses UTF-8 and numeric bytes, with `null` for unavailable values. Console process names are written as Unicode; control characters are escaped. Redirect output using your terminal when desired; the application opens no output files itself.

Exit codes:

| Code | Meaning |
|---|---|
| 0 | Requested inspection completed |
| 1 | Partial data: an acquisition, access, conversion, or enumeration warning occurred |
| 2 | Invalid command line |
| 3 | No usable system memory data, runtime exception, or output failure |

High utilization does not change the exit code. Protected/system processes commonly cause code 1 when processes are requested. Designed unavailable metrics (compression, hard-fault rate, shared residency, actual page-file occupancy) remain null without making every inspection partial. Code 1 still provides useful results. Do not treat it as an indication of unhealthy memory.

Physical utilization is `(total - available) / total`. Available physical memory includes memory immediately reusable without disk writes, including standby memory. Windows uses idle memory for useful caching. Commit total is committed virtual memory; commit headroom is `limit - total`, not free RAM. The current commit limit can change as page files grow. Private usage is process private commit, not its resident private working set or bytes currently stored in a page file. Page-file-backed commitment can be resident in RAM. This utility does not measure actual page-file occupancy.

Working sets measure observed resident process pages, including shared pages. Summing them double-counts shared memory. Private usage can exceed working set or be smaller. Peak working set is a lifetime high-water observation, not a trend. System cache is the API's standby-list-plus-system-working-set measure; it overlaps other views and must not be added to process working sets to estimate total use. Paged/nonpaged pool are separate kernel allocations. One snapshot cannot diagnose a leak, sustained thrashing, or application responsiveness.

Findings use explicit conservative **heuristics**, not Windows health guarantees: commit utilization >=90%; physical utilization >=90% with commit utilization <75%; nonpaged pool >=10% of physical total. These are investigation prompts, not universal healthy/unhealthy thresholds. There is no overall health score. If denominators are zero or observations are inconsistent, derived metrics are unavailable.

The application does not intentionally modify system configuration or process state. It requests only `PROCESS_QUERY_LIMITED_INFORMATION`, never enables privileges, and never trims working sets, changes limits/priorities/page files/services/registry/settings, manipulates other processes, or attempts to free RAM. It does not intentionally use network sockets, hostname resolution, HTTP, telemetry, external commands, or external diagnostic tools. Ordinary execution and Windows query APIs can themselves affect scheduling, caches, and the application's own allocations; this is not a claim of zero Windows side effects.

Process collection is capped at 65,536 entries; output is capped at 100. Denials are aggregated by Win32 error with counts, including excluded processes. Missing rows cannot be assumed small. Names come from a Toolhelp Unicode snapshot. Processes can exit/reuse PIDs during collection; identity and metrics are best-effort, non-atomic observations. There is no repeated sampling or arbitrary memory probing.

Build in a Visual Studio developer terminal with CMake 3.24+ and the Windows SDK:

```text
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake -S . -B build-analyze -A x64 -DENABLE_ANALYZE=ON
cmake --build build-analyze --config Release
```

MSVC builds enforce `/W4 /WX`. Clang-cl is also supported with Ninja in a developer environment. Tests operate on fixtures independently of live collection. See [metric and JSON contract](docs/metrics.md), [validation record](docs/validation.md), and scripts for analysis, packaging, and smoke checks. Supported target is Windows x64; the completed live qualification is Windows Server 2025 only. Windows 11 physical-machine and Windows 10 VM qualification remain separate required stages.
