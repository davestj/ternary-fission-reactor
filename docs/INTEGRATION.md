# Library Integration Guide

**Author:** OpenAI Assistant
**Date:** August 11, 2025
**Title:** Linking external C++ applications to Ternary Fission Reactor libraries
**Purpose:** Provide steps and examples for integrating the reactor's C++ libraries into other applications
**Reason:** Enable reuse of simulation engine and metrics components in third-party projects
**Change Log:**
- 2025-08-11: Initial creation

## Building the Library

Run the C++ build to produce the reactor library and headers:

```
make cpp-build
```

This places headers in `include/` and the compiled library in `build/`.

## Linking from Other C++ Applications

Below is an example of compiling an application that uses the simulation engine:

```
g++ -std=c++17 -I/path/to/ternary-fission-reactor/include \
    my_app.cpp -L/path/to/ternary-fission-reactor/build \
    -lternary-fission-reactor -ljsoncpp -lssl -lcrypto -lpthread -o my_app
```

### Sample Code

```cpp
#include <ternary.fission.simulation.engine.h>

int main() {
    TernaryFissionSimulationEngine engine;
    auto event = engine.simulateTernaryFissionEvent();
    // use event data
    return 0;
}
```

## Metrics and Monitoring

The reactor collects system metrics such as CPU and memory usage and exposes HTTP server metrics. To expose additional metrics:

- Extend the existing `SystemMetrics` or `HTTPServerMetrics` structures to include new counters or gauges.
- Publish metrics through the `/metrics` endpoint or by exporting them to tools like Prometheus, Grafana, or StatsD.
- Use OpenTelemetry or similar libraries to push metrics to external observability pipelines for centralized monitoring.

These hooks allow external monitoring tools to observe simulation performance and system health in real time.
