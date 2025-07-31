# Ternary Fission Reactor - Usage Guide

**Author:** bthlops (David StJ)  
**Date:** July 30, 2025  
**Title:** Complete usage guide for ternary fission simulation  
**Purpose:** Comprehensive examples from basic to extreme simulation scenarios  

## Quick Reference

```bash
# Basic help
./ternary-fission -h

# API server
./ternary-api -config configs/ternary_fission.conf -port 8080
```

## Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `-h, --help` | Show help message | - |
| `-p, --parent <mass>` | Parent nucleus mass (AMU) | 235.0 |
| `-e, --excitation <MeV>` | Excitation energy | 6.5 |
| `-n, --events <N>` | Number of events | 10 |
| `-t, --threads <N>` | Worker threads | auto |
| `-c, --continuous` | Continuous mode | disabled |
| `-d, --duration <sec>` | Duration (continuous) | 10 |
| `-r, --rate <N>` | Events per second | 10 |
| `-j, --json [file]` | JSON output | simulation_stats.json |
| `-x, --repl` | Interactive mode | disabled |
| `-l, --logdir <path>` | Log directory | ./logs |

## Basic Usage Examples

### Simple Simulation
```bash
# Default U-235 simulation (10 events)
./ternary-fission

# U-238 with 100 events
./ternary-fission -n 100 -p 238

# Custom excitation energy
./ternary-fission -n 50 -p 235 -e 7.5
```

### JSON Output
```bash
# Save statistics to JSON
./ternary-fission -n 1000 -j results.json

# Custom JSON filename
./ternary-fission -n 500 -j experiment_data.json
```

## Medium Intensity Simulations

### Multi-threaded Processing
```bash
# Use 8 threads for 5000 events
./ternary-fission -n 5000 -t 8 -p 238 -e 7.1

# Maximum CPU utilization
./ternary-fission -n 10000 -t 16 -p 239 -e 8.0 -j high_intensity.json
```

### Continuous Mode
```bash
# Run for 60 seconds at 25 events/sec
./ternary-fission -c -d 60 -r 25 -t 8

# High rate continuous simulation
./ternary-fission -c -d 120 -r 50 -p 238 -e 7.5 -t 12
```

### Research-Grade Simulations
```bash
# Large dataset generation
./ternary-fission -n 50000 -p 235 -e 6.5 -t 16 -j research_u235.json

# Comparative isotope study
./ternary-fission -n 25000 -p 233 -e 6.0 -t 8 -j u233_data.json
./ternary-fission -n 25000 -p 235 -e 6.5 -t 8 -j u235_data.json  
./ternary-fission -n 25000 -p 238 -e 7.1 -t 8 -j u238_data.json
```

## Extreme Intensity Simulations

### Maximum Performance
```bash
# Extreme simulation: 100K events, 32 threads, high energy
./ternary-fission -n 100000 -t 32 -p 238 -e 9.5 -j extreme_simulation.json

# Continuous extreme: 10 minutes at maximum rate
./ternary-fission -c -d 600 -r 100 -t 32 -p 239 -e 10.0 -j extreme_continuous.json

# Research cluster simulation
./ternary-fission -n 1000000 -t 64 -p 238 -e 8.5 -j million_events.json
```

### High-Energy Physics Scenarios
```bash
# Very high excitation energy simulation
./ternary-fission -n 50000 -p 238 -e 12.0 -t 24 -j high_energy.json

# Super-heavy element simulation
./ternary-fission -n 25000 -p 252 -e 15.0 -t 16 -j superheavy.json

# Extreme energy with maximum threads
./ternary-fission -n 200000 -p 240 -e 20.0 -t 48 -c -d 1800 -r 75 -j extreme_energy.json
```

### Production Batch Processing
```bash
# 24-hour continuous simulation
./ternary-fission -c -d 86400 -r 30 -t 24 -p 238 -e 7.8 -j daily_production.json

# High-throughput data generation
./ternary-fission -n 5000000 -t 64 -p 235 -e 6.5 -j production_dataset.json
```

## Interactive Mode Examples

### REPL Commands
```bash
# Start interactive mode
./ternary-fission -x

# Commands in REPL:
> event 1        # Simulate 1 event
> event 10       # Simulate 10 events  
> status         # Show system status
> stats          # Show JSON statistics
> json output.json # Save stats to file
> exit           # Quit
```

### Interactive Workflows
```bash
# Interactive exploration
./ternary-fission -x
> event 5                    # Test with 5 events
> stats                      # Check results
> event 100                  # Scale up to 100
> json initial_test.json     # Save results
> event 1000                 # Production run
> json final_results.json    # Save final data
> exit
```

## API Server Usage

### Starting the Server
```bash
# Basic API server
./ternary-api -config configs/ternary_fission.conf -port 8080

# Custom configuration
./ternary-api -config custom.conf -port 9090 -log-level debug

# Production deployment
./ternary-api -config production.conf -port 8080 > api.log 2>&1 &
```

### API Endpoints Testing
```bash
# Test server health
curl http://localhost:8080/api/v1/health

# Get system status
curl http://localhost:8080/api/v1/status | jq

# Create energy field
curl -X POST http://localhost:8080/api/v1/energy-fields \
  -H "Content-Type: application/json" \
  -d '{"initial_energy_mev": 500.0, "auto_dissipate": true}'

# WebSocket monitoring
wscat -c ws://localhost:8080/api/v1/ws/monitor
```

## Performance Optimization

### Memory Management
```bash
# Monitor memory usage during simulation
./ternary-fission -n 100000 -t 16 -l /tmp/logs &
watch -n 1 'ps aux | grep ternary-fission'

# Large simulation with memory tracking
time ./ternary-fission -n 500000 -t 32 -j memory_test.json
```

### CPU Optimization
```bash
# Test optimal thread count
for threads in 4 8 16 24 32; do
  echo "Testing $threads threads"
  time ./ternary-fission -n 10000 -t $threads -j "test_${threads}t.json"
done

# CPU affinity for extreme performance
taskset -c 0-31 ./ternary-fission -n 1000000 -t 32 -j affinity_test.json
```

## Benchmarking Examples

### Performance Testing
```bash
# Standard benchmark
./ternary-fission -n 10000 -t 8 -j benchmark_standard.json

# Speed test various configurations
time ./ternary-fission -n 50000 -t 16 -p 235    # U-235
time ./ternary-fission -n 50000 -t 16 -p 238    # U-238  
time ./ternary-fission -n 50000 -t 16 -p 239    # Pu-239

# Continuous throughput test
./ternary-fission -c -d 300 -r 100 -t 32 -j throughput_test.json
```

### Scaling Analysis
```bash
# Event count scaling
for events in 1000 5000 10000 50000 100000; do
  time ./ternary-fission -n $events -t 16 -j "scale_${events}.json"
done

# Thread scaling analysis
for threads in 1 2 4 8 16 32; do
  time ./ternary-fission -n 25000 -t $threads -j "thread_${threads}.json"
done
```

## Production Workflows

### Data Collection Pipeline
```bash
#!/bin/bash
# production_run.sh - Automated data collection

# Morning collection (8 hours)
./ternary-fission -c -d 28800 -r 25 -t 16 -j morning_$(date +%Y%m%d).json &

# Wait and collect evening data
sleep 28800
./ternary-fission -c -d 28800 -r 25 -t 16 -j evening_$(date +%Y%m%d).json
```

### Batch Processing
```bash
#!/bin/bash
# batch_isotopes.sh - Multi-isotope analysis

isotopes=(233 235 238 239 241)
energies=(6.0 6.5 7.1 7.8 8.2)

for i in "${!isotopes[@]}"; do
  echo "Processing U-${isotopes[$i]}"
  ./ternary-fission -n 100000 -p ${isotopes[$i]} -e ${energies[$i]} \
    -t 24 -j "batch_u${isotopes[$i]}.json"
done
```

## Error Handling and Troubleshooting

### Debug Mode
```bash
# Verbose logging
./ternary-fission -n 100 -l /tmp/debug_logs

# Check log output
tail -f /tmp/debug_logs/event.log

# Memory debugging
valgrind ./ternary-fission -n 10 --help
```

### Performance Issues
```bash
# Check system resources
htop                    # Monitor CPU/memory
iotop                   # Monitor disk I/O
nvidia-smi              # Monitor GPU (if applicable)

# Test with reduced load
./ternary-fission -n 1000 -t 4 -r 5    # Reduced intensity
```

## Integration Examples

### With Docker
```bash
# Run in container
docker run -it ternary-fission ./ternary-fission -n 1000 -j /data/results.json

# API server in container  
docker run -d -p 8080:8080 ternary-fission ./ternary-api -port 8080
```

### With Kubernetes
```bash
# Batch job
kubectl create job ternary-sim --image=ternary-fission \
  -- ./ternary-fission -n 100000 -t 16 -j /results/batch.json

# Service deployment
kubectl create deployment ternary-api --image=ternary-fission \
  -- ./ternary-api -config /config/production.conf
```

## Advanced Configuration

### Custom Configuration Files
```bash
# Use custom config
./ternary-api -config /etc/ternary/production.conf -port 8080

# Override specific settings
./ternary-fission -n 10000 -l /var/log/ternary -j /data/results.json
```

### Environment Variables
```bash
# Set environment overrides
export TERNARY_THREADS=32
export TERNARY_LOG_LEVEL=debug
./ternary-fission -n 50000
```

## Validation and Verification

### Conservation Law Checking
```bash
# Check conservation laws
./ternary-fission -n 1000 -j conservation_test.json
grep "Conservation:" logs/event.log | grep "FAIL"

# Physics validation
./ternary-fission -n 10000 -p 235 -e 6.5 -j validation.json
```

### Comparison Studies
```bash
# Cross-validation with known data
./ternary-fission -n 25000 -p 235 -e 6.5 -j experimental.json
./ternary-fission -n 25000 -p 235 -e 6.5 -j validation.json

# Statistical comparison
python compare_results.py experimental.json validation.json
```

---

**For questions or support:**
- Email: davestj@gmail.com
- GitHub: https://github.com/davestj/ternary-fission-reactor
- Website: https://davestj.com
- Stargate Framework: https://davestj.com/bthl/stargate_framework/
