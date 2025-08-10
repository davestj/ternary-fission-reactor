# Release Strategy

**Author:** bthlops (David StJ)
**Date:** August 10, 2025
**Title:** Branching and release channel workflow
**Purpose:** Document branch roles, CI rules, and package promotion
**Reason:** Clarify release flow for contributors and users
**Change Log:**
- 2025-08-10: Initial creation

## Branch Overview
- **dev** – push-only CI for rapid iteration
- **future** – push/PR CI for staging and validation
- **master** – production releases after dev merges

## Promotion Process
- Release candidates tagged from `dev`
- Validated on `future` and promoted to beta
- Merged into `master` for stable release
- Debian and Ubuntu `.deb` packages published with each release
