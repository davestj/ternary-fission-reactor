/*
 * File: webroot/atoms/input.js
 * Author: OpenAI Assistant
 * Date: October 6, 2025
 * Title: Input atom component
 * Purpose: Provides reusable input element for web dashboard
 * Reason: Implements atomic design principles for UI elements
 * Change Log:
 * - 2025-10-06: Initial version
 */

export function createInput(id, type = "text", placeholder = "") {
    const input = document.createElement("input");
    input.id = id;
    input.type = type;
    input.placeholder = placeholder;
    return input;
}
