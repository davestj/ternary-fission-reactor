/*
 * File: src/go/api_integration_test.go
 * Author: OpenAI
 * Date: 2025-02-15
 * Title: API integration tests for reactor forwarding
 * Purpose: Ensure API server forwards requests to reactor stub and parses responses
 * Reason: Provides regression coverage for energy field and status endpoints
 *
 * Change Log:
 * 2025-02-15: Initial tests for energy field forwarding and status parsing
 */

package main

import (
    "encoding/json"
    "io"
    "net/http"
    "net/http/httptest"
    "testing"
)

// TestListEnergyFieldsForwards verifies that the API forwards energy field requests to the reactor.
func TestListEnergyFieldsForwards(t *testing.T) {
    called := false
    stub := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
        if r.URL.Path != "/api/v1/energy-fields" {
            t.Fatalf("unexpected path: %s", r.URL.Path)
        }
        called = true
        w.Header().Set("Content-Type", "application/json")
        w.Write([]byte(`[{"id":"abc"}]`))
    }))
    defer stub.Close()

    cfg := &Config{
        ReactorBaseURL:    stub.URL,
        APITimeout:        5,
        PrometheusEnabled: false,
    }

    api := NewTernaryFissionAPIServer(cfg)
    server := httptest.NewServer(api.router)
    defer server.Close()

    resp, err := http.Get(server.URL + "/api/v1/energy-fields")
    if err != nil {
        t.Fatalf("request failed: %v", err)
    }
    defer resp.Body.Close()

    if !called {
        t.Fatalf("stub not called")
    }

    body, _ := io.ReadAll(resp.Body)
    if string(body) != `[{"id":"abc"}]` {
        t.Fatalf("unexpected body: %s", string(body))
    }
}

// TestGetSystemStatusParsesResponse ensures status endpoint parses reactor response correctly.
func TestGetSystemStatusParsesResponse(t *testing.T) {
    expected := SystemStatusResponse{
        UptimeSeconds:        10,
        TotalFissionEvents:   3,
        TotalEnergySimulated: 4.5,
        ActiveEnergyFields:   1,
        PeakMemoryUsage:      0,
        AverageCalcTime:      0,
        TotalCalculations:    0,
        SimulationRunning:    true,
        CPUUsagePercent:      0.7,
        MemoryUsagePercent:   0.2,
    }

    stub := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
        if r.URL.Path != "/api/v1/status" {
            t.Fatalf("unexpected path: %s", r.URL.Path)
        }
        w.Header().Set("Content-Type", "application/json")
        json.NewEncoder(w).Encode(expected)
    }))
    defer stub.Close()

    cfg := &Config{
        ReactorBaseURL:    stub.URL,
        APITimeout:        5,
        PrometheusEnabled: false,
    }

    api := NewTernaryFissionAPIServer(cfg)
    server := httptest.NewServer(api.router)
    defer server.Close()

    resp, err := http.Get(server.URL + "/api/v1/status")
    if err != nil {
        t.Fatalf("request failed: %v", err)
    }
    defer resp.Body.Close()

    var got SystemStatusResponse
    if err := json.NewDecoder(resp.Body).Decode(&got); err != nil {
        t.Fatalf("decode failed: %v", err)
    }

    if got != expected {
        t.Fatalf("unexpected status: %+v", got)
    }
}
