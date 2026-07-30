#pragma once

// ScriptBench phase timer: wall time AFTER load/init seal.
// Prints to stderr so stdout goldens stay clean.
// Future: seal at build/link time so this phase is the whole process.

#include <chrono>
#include <cstdio>

namespace zefc {
namespace smoke {

struct BenchPhase {
  const char* name;
  std::chrono::steady_clock::time_point t0;

  explicit BenchPhase(const char* phase_name)
    : name(phase_name)
    , t0(std::chrono::steady_clock::now())
  {
    std::fprintf(stderr,
                 "zefc-bench: timing '%s' AFTER STARTUP "
                 "(post zefc_vtables_seal; excludes package/selector init)\n",
                 name);
    std::fflush(stderr);
  }

  BenchPhase(const BenchPhase&) = delete;
  BenchPhase& operator=(const BenchPhase&) = delete;

  ~BenchPhase()
  {
    const auto t1 = std::chrono::steady_clock::now();
    const double secs = std::chrono::duration<double>(t1 - t0).count();
    std::fprintf(stderr, "zefc-bench: '%s' AFTER STARTUP: %.6f s\n", name, secs);
    std::fflush(stderr);
  }
};

} // namespace smoke
} // namespace zefc
