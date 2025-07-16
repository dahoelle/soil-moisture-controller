#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>NOICE, MOIST !</title>
  <style>
    body {
      margin: 0;
      font-family: sans-serif;
      background-color: #1a1a1a;
      color: #ccc;
      display: flex;
      height: 100vh;
      overflow: hidden;
    }

    .sidebar {
      width: 60px;
      background-color: #111;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 1rem 0;
    }

    .sidebar button {
      background: none;
      border: none;
      color: #777;
      font-size: 1.5rem;
      margin: 1rem 0;
      cursor: pointer;
      transition: color 0.2s;
    }

    .sidebar button.active {
      color: #fff;
    }

    .main {
      flex: 1;
      display: flex;
      flex-direction: column;
    }

    header {
      background: #222;
      padding: 1rem;
      display: flex;
      align-items: center;
    }

    header h1 {
      margin: 0;
      font-size: 1.2rem;
    }

    .panel {
      display: none;
      padding: 2rem;
      overflow-y: auto;
      flex: 1;
    }

    .panel.active {
      display: block;
    }

    .config-section {
      background: #2a2a2a;
      padding: 1rem 1.5rem;
      border-radius: 8px;
      max-width: 500px;
      box-sizing: border-box;
    }

    .config-section h2 {
      margin-top: 0;
    }

    .row {
      margin: 1rem 0;
    }

    input {
      width: 100%;
      padding: 0.5rem;
      padding-right: 2rem;
      background: #1e1e1e;
      border: 1px solid #444;
      color: #ccc;
      box-sizing: border-box;
    }

    button.save {
      background: #444;
      color: #eee;
      border: none;
      padding: 0.5rem 1rem;
      cursor: pointer;
      margin-right: 0.5rem;
    }

    button.danger {
      background: #b33;
      color: #fff;
    }
  </style>
</head>
<body>

  <div class="sidebar">
    <button id="btn-home" class="active" onclick="showPanel('home')">🏠</button>
    <button id="btn-hydration" onclick="showPanel('hydration')">💧</button>
    <button id="btn-graph" onclick="showPanel('graph')">📈</button>
    <button id="btn-editor" onclick="showPanel('editor')">📝</button>
    <button id="btn-config" onclick="showPanel('config')">⚙️</button>
  </div>

  <div class="main">
    <header>
      <h1>NOICE, MOIST !</h1>
    </header>

    <div id="home-panel" class="panel active">
      <div class="config-section">
        <h2>LED Control</h2>
        <div class="row">
          <label for="r">Red:</label>
          <input type="number" id="r" min="0" max="255" />
        </div>
        <div class="row">
          <label for="g">Green:</label>
          <input type="number" id="g" min="0" max="255" />
        </div>
        <div class="row">
          <label for="b">Blue:</label>
          <input type="number" id="b" min="0" max="255" />
        </div>
        <button class="save" id="led-set">Set LED</button>
        <button class="save" id="led-off">Turn OFF</button>
      </div>
    </div>

    <div id="hydration-panel" class="panel">
      <p>Hydration panel content</p>
    </div>

    <div id="graph-panel" class="panel">
      <!-- Content loaded dynamically -->
      <div id="graph-content">Loading graph...</div>
    </div>

    <div id="editor-panel" class="panel">
      <!-- Content loaded dynamically -->
      <div id="editor-content">Loading editor...</div>
    </div>

    <div id="config-panel" class="panel">
      <div class="config-section">
        <h2>WiFi Config</h2>
        <div class="row">
          <label for="ssid">SSID:</label>
          <input type="text" id="ssid" />
        </div>
        <div class="row">
          <label for="pass">Password:</label>
          <input type="password" id="pass" />
        </div>
        <button class="save" id="wifi-save">Save</button>
      </div>

      <div class="config-section" style="margin-top:2rem;">
        <h2>Maintenance</h2>
        <button class="save danger" id="clear-logs">Clear Logs</button>
        <button class="save danger" id="restart-device">Restart Device</button>
      </div>
    </div>
  </div>

  <script>
    function showPanel(name) {
      document.querySelectorAll(".panel").forEach(p => p.classList.remove("active"));
      document.getElementById(name + "-panel").classList.add("active");

      document.querySelectorAll(".sidebar button").forEach(b => b.classList.remove("active"));
      document.getElementById("btn-" + name).classList.add("active");

      if (name === "graph") {
        fetchGraphContent();
      } else if (name === "editor") {
        fetchEditorContent();
      }
    }

    function fetchGraphContent() {
      fetch("/graph")
        .then(res => {
          if (!res.ok) throw new Error(`HTTP ${res.status}`);
          return res.text();
        })
        .then(html => {
          document.getElementById("graph-content").innerHTML = html;
        })
        .catch(err => {
          console.error("Error loading graph content:", err);
          document.getElementById("graph-content").innerHTML = "<p>Failed to load graph content.</p>";
        });
    }

    function fetchEditorContent() {
      fetch("/editor")
        .then(res => {
          if (!res.ok) throw new Error(`HTTP ${res.status}`);
          return res.text();
        })
        .then(html => {
          document.getElementById("editor-content").innerHTML = html;
        })
        .catch(err => {
          console.error("Error loading editor content:", err);
          document.getElementById("editor-content").innerHTML = "<p>Failed to load editor content.</p>";
        });
    }

    function updateWifiConfig() {
      const ssid = document.getElementById("ssid").value;
      const pass = document.getElementById("pass").value;

      fetch("/wifi-config/", {
        method: "PUT",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ ssid, pass })
      })
      .then(res => {
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        return res.json();
      })
      .then(data => {
        console.log("WiFi config updated:", data);
        alert("WiFi settings saved.");
      })
      .catch(err => {
        console.error("WiFi config error:", err);
        alert("Failed to update WiFi config: " + err.message);
      });
    }

    function updateLED() {
      const r = parseInt(document.getElementById("r").value || 0);
      const g = parseInt(document.getElementById("g").value || 0);
      const b = parseInt(document.getElementById("b").value || 0);

      fetch("/led/", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ r, g, b })
      })
      .then(res => res.json())
      .then(data => {
        console.log("LED updated:", data);
        alert("LED color set.");
      })
      .catch(err => {
        console.error("LED update error:", err);
        alert("Failed to update LED: " + err.message);
      });
    }

    function turnOffLED() {
      fetch("/led/", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: "{}"
      })
      .then(res => res.json())
      .then(data => {
        console.log("LED turned off:", data);
        alert("LED turned off.");
      })
      .catch(err => {
        console.error("LED off error:", err);
        alert("Failed to turn off LED: " + err.message);
      });
    }

    function clearLogs() {
      if (!confirm("Are you sure you want to clear all logs? This action cannot be undone.")) return;

      fetch("/clear-logs/", {
        method: "POST"
      })
      .then(res => {
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        return res.json();
      })
      .then(data => {
        alert(data.status || "Logs cleared.");
      })
      .catch(err => {
        console.error("Clear logs error:", err);
        alert("Failed to clear logs: " + err.message);
      });
    }

    function restartDevice() {
      if (!confirm("Are you sure you want to restart the device?")) return;

      fetch("/restart/", {
        method: "POST"
      })
      .then(res => {
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        return res.json();
      })
      .then(data => {
        alert(data.status || "Device restarting...");
      })
      .catch(err => {
        console.error("Restart error:", err);
        alert("Failed to restart device: " + err.message);
      });
    }

    document.addEventListener("DOMContentLoaded", () => {
      document.getElementById("wifi-save").addEventListener("click", updateWifiConfig);
      document.getElementById("led-set").addEventListener("click", updateLED);
      document.getElementById("led-off").addEventListener("click", turnOffLED);
      document.getElementById("clear-logs").addEventListener("click", clearLogs);
      document.getElementById("restart-device").addEventListener("click", restartDevice);
    });
  </script>

</body>
</html>
)rawliteral";

#endif
