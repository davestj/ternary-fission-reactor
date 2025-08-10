/*
 * File: webroot/organisms/navBar.js
 * Author: OpenAI Assistant
 * Date: October 6, 2025
 * Title: Navigation bar organism
 * Purpose: Provides application navigation and management controls
 * Reason: Centralized user actions including logout and shutdown
 * Change Log:
 * - 2025-10-06: Initial version
 */

import { createButton } from "/atoms/button.js";

export function createNavBar() {
    const nav = document.createElement("nav");
    const home = document.createElement("a");
    home.href = "/index.html";
    home.textContent = "Dashboard";

    const shutdownBtn = createButton("Shutdown");
    shutdownBtn.addEventListener("click", async () => {
        await fetch("/api/v1/control/shutdown", {method: "POST"});
    });

    const logoutBtn = createButton("Logout");
    logoutBtn.addEventListener("click", () => {
        sessionStorage.removeItem("token");
        window.location.href = "/login.html";
    });

    nav.appendChild(home);
    nav.appendChild(shutdownBtn);
    nav.appendChild(logoutBtn);
    return nav;
}
