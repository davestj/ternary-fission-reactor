# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Dashboard: real-time charts for monitoring.
- Integration tests and `go-test` target for API endpoints.
- Prometheus metrics polling for reactor status.

### Documentation
- Expand README with build and run instructions.

## [1.1.13] – 2025-08-09

### Added
- CLI application entry point.
- API proxy for energy field requests and dissipation handler.
- Reactor base URL configuration option.
- Web and media root configuration examples.

### Fixed
- Makefile conditional structure.
- Unused variable warning in WebSocket status broadcast.
- Release package permission issue.
- Typographical errors.

### Documentation
- Document web_root and media_root options.

[Unreleased]: https://github.com/davestj/ternary-fission-reactor/compare/v1.1.13...HEAD
[1.1.13]: https://github.com/davestj/ternary-fission-reactor/releases/tag/v1.1.13
