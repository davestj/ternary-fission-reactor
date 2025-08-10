/*
 * File: webroot/molecules/statusCard.js
 * Author: OpenAI Assistant
 * Date: October 6, 2025
 * Title: Status card molecule
 * Purpose: Displays labeled value for metrics
 * Reason: Reusable component for monitoring data
 * Change Log:
 * - 2025-10-06: Initial version
 */

export function createStatusCard(label, initial = "...") {
    const card = document.createElement("div");
    card.className = "status-card";
    const title = document.createElement("h3");
    title.textContent = label;
    const value = document.createElement("p");
    value.textContent = initial;
    card.appendChild(title);
    card.appendChild(value);
    return {card, update: (val) => {value.textContent = val;}};
}
