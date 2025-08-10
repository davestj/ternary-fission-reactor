/*
 * File: webroot/organisms/metricsPanel.js
 * Author: OpenAI Assistant
 * Date: October 6, 2025
 * Title: Metrics panel organism
 * Purpose: Displays real-time metrics with periodic fetching
 * Reason: Provides monitoring interface for reactor data
 * Change Log:
 * - 2025-10-06: Initial version
 */

import { createStatusCard } from "/molecules/statusCard.js";

export function createMetricsPanel() {
    const panel = document.createElement("div");
    const power = createStatusCard("Power", "0 MW");
    const temp = createStatusCard("Temperature", "0 C");
    panel.appendChild(power.card);
    panel.appendChild(temp.card);

    async function updateMetrics() {
        const response = await fetch("/api/v1/metrics");
        if (response.ok) {
            const data = await response.json();
            if (data.power !== undefined) {
                power.update(`${data.power} MW`);
            }
            if (data.temperature !== undefined) {
                temp.update(`${data.temperature} C`);
            }
        }
    }

    updateMetrics();
    setInterval(updateMetrics, 5000);
    return panel;
}
