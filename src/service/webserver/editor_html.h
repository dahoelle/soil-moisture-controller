#ifndef EDITOR_HTML_H
#define EDITOR_HTML_H

#include <Arduino.h>

const char EDITOR_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Config Editor</title>
  <style>
    html, body { margin: 0; height: 100%; }
    #editor { width: 100%; height: 90vh; }
    button { width: 100%; padding: 1rem; font-size: 1rem; }
  </style>
</head>
<body>
  <div id="editor"></div>
  <button onclick="saveConfig()">Save Config</button>

  <script src="https://cdnjs.cloudflare.com/ajax/libs/ace/1.4.14/ace.js"></script>
  <script>
    const editor = ace.edit("editor");
    editor.session.setMode("ace/mode/json");
    editor.setTheme("ace/theme/monokai");

    // Load config.json by GET from /api/file/config.json (no body)
    function loadConfig() {
      fetch("/api/file/config.json")
      .then(res => {
        if (!res.ok) throw new Error("Failed to load config");
        return res.text();
      })
      .then(text => {
        try {
          const json = JSON.parse(text);
          editor.setValue(JSON.stringify(json, null, 2), -1);
        } catch {
          // Not valid JSON, load as plain text
          editor.setValue(text, -1);
        }
      })
      .catch(err => {
        // Start empty if loading fails
        editor.setValue("", -1);
        console.warn("Error loading config:", err.message);
      });
    }

    function saveConfig() {
      const content = editor.getValue();
      try {
        JSON.parse(content);
      } catch(e) {
        alert("Invalid JSON: " + e.message);
        return;
      }

      fetch("/api/file/config.json", {
        method: "PUT",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ content: content })
      })
      .then(res => {
        if (res.ok) {
          alert("Config saved.");
        } else {
          res.text().then(text => alert("Failed to save: " + text));
        }
      })
      .catch(err => {
        alert("Failed to save: " + err.message);
      });
    }

    loadConfig();
  </script>
</body>
</html>
)rawliteral";

#endif
