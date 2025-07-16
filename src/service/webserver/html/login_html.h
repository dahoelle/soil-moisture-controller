#ifndef LOGIN_HTML_H
#define LOGIN_HTML_H

#include <Arduino.h>

const char LOGIN_HTML[] PROGMEM = R"rawliteral(
  <form id="loginForm">
      <input name="name" placeholder="Username">
      <input name="pass" type="password" placeholder="Password">
      <button type="submit">Login</button>
  </form>

  <script>
    document.getElementById('loginForm').addEventListener('submit', async (e) => {
      e.preventDefault();
      const form = e.target;
      const name = form.name.value;
      const pass = form.pass.value;

      const response = await fetch('/auth', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ name, pass })
      });

      const result = await response.json();
      if (response.ok) {
        alert('Login successful');
      } else {
        alert('Login failed: ' + (result.error || 'unknown error'));
      }
    });
  </script>
)rawliteral";

#endif
