# Benchmarking Presets

**Author:** bthlops (David StJ)
**Date:** August 10, 2025
**Title:** Benchmarking presets for event-driven fission simulation runs
**Purpose:** Provide standardized presets for repeatable benchmarking
**Reason:** Simplify performance comparison across common scenarios
**Change Log:**
- 2025-08-10: Initial creation

| Preset | Events | Duration | Power Multiplier |
|--------|--------|----------|------------------|
| small  | 1,000  | 10 s     | 1x               |
| medium | 10,000 | 1 m      | 2x               |
| large  | 100,000 | 5 m     | 5x               |
| xlarge | 1,000,000 | 30 m  | 10x              |

Each preset configures the simulation with a predefined number of events, run time, and power multiplier. Use these settings to ensure consistent comparisons across runs.

Example usage:

```bash
# Run with the medium preset
./ternary-fission --preset medium
```
