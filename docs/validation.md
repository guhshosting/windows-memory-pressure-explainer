# Validation record — 1.0.0

> Release preparation update (2026-10-05): see [current executable qualification and package provenance](../RELEASE-QUALIFICATION.md). Historical validation and pending-test statements in this document describe the original development snapshot. The final ZIP is assembled after qualification and has not been run on Windows.


Performed 2026-10-02 on Windows Server 2025 Standard Evaluation, 24H2, build 26100 (registry UBR 32230), x64. This record covers this development host only. No Windows 11 or Windows 10 execution is claimed.

## Results

| Check | Result |
|---|---|
| Clean MSVC Release x64 build | Passed, MSVC 19.51.36260.0, `/W4 /WX`, static runtime |
| Full Release CTest | 4/4 passed: unit, help, version, independent JSON contract |
| MSVC `/analyze` clean build | Passed; no project diagnostics, warnings treated as errors |
| clang-tidy 23.1.2 | Passed: clang-analyzer, bugprone, performance, portability checks; project warnings treated as errors |
| Cppcheck 2.22.0 | Passed: warning, style, performance, portability; C++17, win64 |
| Clang-cl 23.1.2 Release build | Passed with `/W4 /WX`; 4/4 CTest passed |
| Source/import safety review | Passed with runtime-import qualifications below |
| Embedded manifest | asInvoker, uiAccess=false |
| Unelevated real-host inspection | summary, processes, explain, JSON, help, version, invalid usage passed |
| Independent JSON parsing | PowerShell ConvertFrom-Json parsed fixtures and live output |
| Portable ZIP extraction | Fresh directory extraction and exact packaged executable smoke-tested |
| SHA-256 | Generated for the final ZIP; extracted executable matches built executable hash |

The live token was medium integrity, BUILTIN Users, without enabled administrator membership. MSVC builds needed permission to access build-tool file-tracking services outside the execution sandbox; diagnostic live and packaged runs used the normal unelevated sandbox token. Build permissions were not used to elevate the application. Host details and token evidence are saved in `artifacts/host.txt` and `artifacts/token.txt`.

Live summary returned exit 0 and complete core system values. Processes/explain returned exit 1 due to expected inaccessible system/other-user processes, including Win32 access denied and PID 0 invalid parameter. Accessible consumers were ranked with distinct working-set/private-usage fields; output remained at top 3 in smoke checks. Findings did not determine exit status. Invalid `--top 101` returned 2. The binary created no output files itself; validation scripts explicitly captured output.

Fixture checks cover page multiplication overflow/zero/max values; headroom and percentages including inconsistent/missing data; parsing defaults, duplicates, commands, bounds and malformed options; deterministic process ordering and top truncation; null/zero JSON semantics; control-character and Unicode escaping/roundtrip; stable schema version and findings; separate working-set/private-usage values. These tests do not require generating memory pressure, manipulating processes, or forcing real acquisition failures. System/process observations are collected into a Snapshot model so calculation, ranking, serialization and findings can be tested independently.

clang-tidy prints generated warnings from the Windows SDK/MSVC standard library that it suppresses as non-user code by default (1196 in this run); there were no project diagnostics. No check categories or project diagnostics were blanket-disabled and no source NOLINT suppressions are used. Cppcheck does not have a full Windows SDK semantic model; successful real compiler builds and clang-tidy supplement it. No sanitizers were run.

## Safety and network review

Application source manually reviewed along with forbidden-API searches and executable imports. Acquisition calls: GetPerformanceInfo, GlobalMemoryStatusEx fallback, CreateToolhelp32Snapshot, Process32FirstW/NextW, OpenProcess with only PROCESS_QUERY_LIMITED_INFORMATION, GetProcessMemoryInfo, CloseHandle. Unicode output uses conversion APIs, GetStdHandle/GetConsoleMode and WriteConsoleW. RAII closes owned handles; standard output handles are borrowed. No privileges are requested or enabled. The only allocations are normal bounded program/observation/serialization allocations, never an attempt to produce pressure. Collection is capped at 65,536 process entries and displayed consumers at 100.

The final executable imports KERNEL32.dll only. Its statically linked C++ runtime brings general imports such as CreateFileW, WriteFile, TerminateProcess, VirtualProtect, LoadLibraryExW, GetProcAddress, and SetEnvironmentVariableW. These are **not** proof of application-level external mutation: TerminateProcess is runtime self-process failure handling, protection/loader operations support the application's own runtime, and file/console operations support standard streams. Application source contains no calls to these process-mutating/file-opening APIs and no shell execution or configuration-changing code. This audit distinguishes ordinary runtime capabilities from application query behavior rather than claiming the binary contains no state-changing imports at all.

No socket, DNS, HTTP, WinINet/WinHTTP, service, or registry-mutation imports were found. Source contains no networking implementation, dynamic network-library loading, command construction/execution, telemetry, or external-tool invocation. Developer build/test scripts run compilers, analyzers and PowerShell; the diagnostic executable does not. This is a source/import inspection, not packet-level proof. No packet capture or kernel-level API tracing was performed. The program does not intentionally modify system configuration or another process's state; no claim of zero Windows side effects is made.

## Reproduction and evidence

Use README build commands, `scripts/clang-validation.ps1` (adjust its host toolchain paths for another machine), `scripts/smoke.ps1`, and `scripts/package.ps1`. Cppcheck command:

```text
cppcheck --enable=warning,style,performance,portability --std=c++17 --platform=win64 --inline-suppr --error-exitcode=1 -Isrc src tests
```

Logs in the source workspace: `artifacts/release-build.txt`, `ctest.txt`, `msvc-analyze.txt`, `clang-tidy.txt`, `cppcheck.txt`, `imports.txt`, `query-api-audit.txt`, `forbidden-source-search.txt`, `manifest.xml`, `live-final/`, and `packaged/`. An empty forbidden-source-search file means no matches, not a claim that a text search alone proves safety. Tool results were inspected.

The portable release is `dist/windows-memory-pressure-explainer-1.0.0-windows-x64.zip`; checksum material is the adjacent `.zip.sha256`. The ZIP includes the executable, README, MIT license and documentation. No debug binaries, test tools or installer are included. The executable is unsigned; signing was not performed.

## Limits and remaining qualification

This is a single non-atomic snapshot, with bounded enumeration and normal token visibility. Exiting/PID-reused processes may race observation. Denied metrics remain unavailable; warning counts cover queried entries including those excluded by top-N. Compression, shared residency, actual page-file occupancy and hard-fault rates are explicitly unavailable. No sustained-pressure, leak, real near-limit, unusual-pool, or low-memory stress experiment was run on this host; the heuristic paths are fixture-tested. Process enumeration failure and system-query fallback were not forced on the live OS. Console Unicode logic compiled and Unicode JSON roundtrips were checked; interactive display across every Windows terminal/code page was not exhaustively qualified.

Remaining release qualification: run the same unelevated executable and tests on a Windows 11 physical machine and a Windows 10 x64 VM. Include Unicode process names, protected/exiting processes, output redirection and terminal behavior, consistency with documented metric definitions, and ordinary workloads. These are separate later qualification stages, not completed by this Server 2025 validation.
