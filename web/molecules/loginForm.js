/*
 * File: web/molecules/loginForm.js
 * Author: OpenAI Assistant
 * Date: October 6, 2025
 * Title: Login form molecule
 * Purpose: Combines input and button atoms into a login form
 * Reason: Implements authentication workflow using atomic components
 * Change Log:
 * - 2025-10-06: Initial version
 */

import { createInput } from "/atoms/input.js";
import { createButton } from "/atoms/button.js";

export function createLoginForm(onSuccess) {
    const form = document.createElement("form");
    const user = createInput("username", "text", "Username");
    const pass = createInput("password", "password", "Password");
    const submit = createButton("Login", "submit");

    form.appendChild(user);
    form.appendChild(pass);
    form.appendChild(submit);

    form.addEventListener("submit", async (e) => {
        e.preventDefault();
        const response = await fetch("/api/v1/login", {
            method: "POST",
            headers: {"Content-Type": "application/json"},
            body: JSON.stringify({username: user.value, password: pass.value})
        });
        if (response.ok) {
            const data = await response.json();
            sessionStorage.setItem("token", data.token || "");
            onSuccess();
        } else {
            alert("Login failed");
        }
    });

    return form;
}
