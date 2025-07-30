// File: src/go/go.mod
// Author: bthlops (David StJ)
// Date: July 29, 2025
// Title: Go Module Configuration for Ternary Fission API Server
// Purpose: Defines Go module dependencies and version requirements
// Reason: Manages external dependencies for HTTP API, WebSocket, metrics, and concurrent processing
//
// Change Log:
// 2025-07-29: Initial creation with core dependencies for API server functionality
// 2025-07-29: Added WebSocket support via Gorilla WebSocket library
// 2025-07-29: Integrated Prometheus metrics collection and monitoring
// 2025-07-29: Added JSON processing and HTTP routing dependencies
// 2025-07-29: Included security and cryptographic libraries for energy simulation
//
// Carry-over Context:
// - This module configuration supports the Go API server component
// - Dependencies include HTTP routing, WebSocket communication, and metrics collection
// - Version constraints ensure compatibility with Go 1.23+ requirements
// - Security libraries support encryption-based energy dissipation simulation
// - All dependencies are production-ready and actively maintained

module ternary-fission

go 1.23

require (
	github.com/gorilla/mux v1.8.1
	github.com/gorilla/websocket v1.5.1
	github.com/prometheus/client_golang v1.19.1
)

require (
	github.com/beorn7/perks v1.0.1 // indirect
	github.com/cespare/xxhash/v2 v2.3.0 // indirect
	github.com/munnerz/goautoneg v0.0.0-20191010083416-a7dc8b61c822 // indirect
	github.com/prometheus/client_model v0.6.1 // indirect
	github.com/prometheus/common v0.55.0 // indirect
	github.com/prometheus/procfs v0.15.1 // indirect
	golang.org/x/net v0.26.0 // indirect
	golang.org/x/sys v0.21.0 // indirect
	google.golang.org/protobuf v1.34.2 // indirect
)
