# Field Deployment Studies

**Author:** bthlops (David StJ)
**Date:** February 14, 2025
**Title:** Field deployment guidelines for triangular Raspberry Pi networks using SDR and audio probes
**Purpose:** Provide instructions for deploying the reactor on Raspberry Pi nodes to study environmental responses using audio and SDR methods
**Reason:** Document field study procedures referencing notable investigations and data gathering strategies
**Change Log:**
- 2025-02-14: Initial creation

## Triangular Raspberry Pi Deployment

Deploy three Raspberry Pi nodes in a triangular layout, spacing them evenly to form an equilateral triangle. Each node should run the reactor software with synchronized clocks and shared configuration files to ensure consistent behavior.

## Load Generation and Audio Probing

Use stress utilities to produce CPU and memory load while playing `.wav` or `.mp3` frequency sweeps through the Raspberry Pi audio outputs. The combined system load and acoustic output help reveal coupling effects in the surrounding environment.

## Environmental Probing with SDR and Speakers

Attach USB SDR dongles to each node and capture spectrum data while simultaneously driving speaker outputs. Record both radio frequency and acoustic responses for later analysis. Simple scripts can log RSSI levels and audio feedback to correlate with reactor activity.

## Notable Investigations and Data Gathering

Field studies may draw inspiration from research at Skinwalker Ranch and experiments with harmonic triangles. Log time-stamped sensor readings, audio recordings, and SDR captures. Store data centrally for comparison across nodes, and note any anomalous correlations between geometry, load profiles, and environmental responses.
