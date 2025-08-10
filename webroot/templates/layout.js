/*
 * File: webroot/templates/layout.js
 * Author: OpenAI Assistant
 * Date: October 6, 2025
 * Title: Layout template
 * Purpose: Renders common layout with navigation bar
 * Reason: Promotes reuse across pages
 * Change Log:
 * - 2025-10-06: Initial version
 */

import { createNavBar } from "/organisms/navBar.js";

export function renderLayout(content) {
    const root = document.getElementById("app");
    root.innerHTML = "";
    const nav = createNavBar();
    root.appendChild(nav);
    root.appendChild(content);
}
