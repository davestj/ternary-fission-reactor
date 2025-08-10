/*
 * File: web/atoms/button.js
 * Author: OpenAI Assistant
 * Date: October 6, 2025
 * Title: Button atom component
 * Purpose: Provides reusable button element for web dashboard
 * Reason: Implements atomic design principles for UI elements
 * Change Log:
 * - 2025-10-06: Initial version
 */

export function createButton(text, type = "button") {
    const button = document.createElement("button");
    button.type = type;
    button.textContent = text;
    return button;
}
