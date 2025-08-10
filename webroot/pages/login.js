/*
 * File: webroot/pages/login.js
 * Author: OpenAI Assistant
 * Date: October 6, 2025
 * Title: Login page script
 * Purpose: Renders login form and handles authentication
 * Reason: Entry point for user login workflow
 * Change Log:
 * - 2025-10-06: Initial version
 */

import { createLoginForm } from "/molecules/loginForm.js";

const app = document.getElementById("app");
const form = createLoginForm(() => {
    window.location.href = "/index.html";
});
app.appendChild(form);
