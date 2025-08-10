/*
 * File: web/pages/index.js
 * Author: OpenAI Assistant
 * Date: October 6, 2025
 * Title: Dashboard page script
 * Purpose: Initializes metrics dashboard layout
 * Reason: Entry point for authenticated monitoring page
 * Change Log:
 * - 2025-10-06: Initial version
 */

import { renderLayout } from "/templates/layout.js";
import { createMetricsPanel } from "/organisms/metricsPanel.js";

function init() {
    if (!sessionStorage.getItem("token")) {
        window.location.href = "/login.html";
        return;
    }
    const panel = createMetricsPanel();
    renderLayout(panel);
}

init();
