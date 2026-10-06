#include "core.h"
#include "output.h"
#include <exception>
#include <cstdio>
#include <iostream>
int main(int argc, char **argv) {
  try {
    std::vector<std::string> args;
    if (argc > 16)
      throw std::invalid_argument("too many arguments");
    for (int i = 1; i < argc; ++i) {
      const std::string a = argv[i];
      if (a.size() > 128)
        throw std::invalid_argument("argument too long");
      args.push_back(a);
    }
    const auto o = wmpe::parse(args);
    if (o.help) {
      std::cout
          << "Usage: windows-memory-pressure-explainer "
             "[summary|processes|explain] [--json] [--top N]\nDefault: "
             "explain; top 10 working sets, maximum 100. All quantities are "
             "bytes unless named otherwise.\n--help | --version (use "
             "alone)\nExit: 0 inspection complete; 1 requested data partially "
             "unavailable; 2 invalid usage; 3 acquisition/runtime/output "
             "failure.\nQuery-only, no elevation, privileges, telemetry, or "
             "network. No leak diagnosis from one snapshot.\n";
      return std::cout ? 0 : 3;
    }
    if (o.version) {
      std::cout << "windows-memory-pressure-explainer 1.0.0\n";
      return std::cout ? 0 : 3;
    }
    auto s = wmpe::acquire(o.command != "summary");
    wmpe::bound(s.processes, o.top);
    const bool written =
        write_output(o.json ? wmpe::json(s, o) : wmpe::human(s, o));
    if (!written || !s.system_ok)
      return 3;
    return s.warnings.empty() ? 0 : 1;
  } catch (const std::invalid_argument &e) {
    std::fprintf(stderr, "Usage error: %s\nUse --help.\n", e.what());
    return 2;
  } catch (const std::exception &e) {
    std::fprintf(stderr, "Runtime failure: %s\n", e.what());
    return 3;
  }
}
