#ifndef EDITOR_HTML_H
#define EDITOR_HTML_H

#include <Arduino.h>

const char EDITOR_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Editor</title>
  <meta charset="UTF-8" />
  <style>
    html, body {
      margin: 0;
      padding: 0;
      width: 100%;
      height: 100%;
      display: flex;
      flex-direction: row;
      overflow: hidden;
      font-family: sans-serif;
      background: #1a1a1a;
      color: #ccc;
    }

    .editor-sidebar {
      width: 100%; /* Fill parent panel width */
      max-width: 300px; /* Optional max width */
      background: #111;
      overflow-y: auto;
      padding: 1rem;
      box-sizing: border-box;
      border-right: 2px solid #333; /* visual aid */
      height: 100%; /* fit parent */
      /* removed min-width */
    }

    .editor-sidebar h2 {
      margin-top: 0;
      font-size: 1.1rem;
      color: #eee;
    }

    .editor-main-panel {
      flex-grow: 1;
      display: flex;
      flex-direction: column;
      background: #222;
      height: 100%;
      min-width: 0;
      border-left: 2px solid #333; /* visual aid */
    }

    .editor-topbar {
      padding: 0.5rem 1rem;
      font-size: 1rem;
      color: #aaa;
      border-bottom: 1px solid #333;
      user-select: none;
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
      flex-shrink: 0;
    }

    #editor-area {
      flex-grow: 1;
      height: 100%;
      min-height: 0;
    }

    .editor-actions {
      padding: 0.5rem;
      background: #222;
      border-top: 1px solid #333;
      flex-shrink: 0;
    }

    .editor-actions button {
      background: #444;
      color: #eee;
      border: none;
      padding: 0.5rem 1rem;
      margin-right: 0.5rem;
      cursor: pointer;
      transition: background 0.2s;
    }

    .editor-actions button:hover {
      background: #555;
    }

    ul {
      list-style-type: none;
      padding-left: 1rem;
      margin: 0;
    }

    .folder > span::before {
      content: '▶';
      display: inline-block;
      width: 1em;
      cursor: pointer;
      user-select: none;
    }

    .folder.open > span::before {
      content: '▼';
    }

    li span {
      cursor: pointer;
      user-select: none;
    }

    li span:hover {
      color: #fff;
      text-decoration: underline;
    }

    .file {
      margin-left: 1.2em;
    }
  </style>
</head>
<body>
  <div class="editor-sidebar">
    <h2>Files</h2>
    <ul id="fileTree"></ul>
  </div>
  <div class="editor-main-panel">
    <div class="editor-topbar" id="filenameBar">No file selected</div>
    <div id="editor-area"></div>
    <div class="editor-actions">
      <button onclick="saveFile()">Save</button>
    </div>
  </div>

  <script src="https://cdnjs.cloudflare.com/ajax/libs/ace/1.4.14/ace.js"></script>
  <script>
    const editor = ace.edit("editor-area");
    editor.setTheme("ace/theme/monokai");
    editor.session.setMode("ace/mode/json");

    let currentFile = "";
    let lastContent = "";

    async function loadTree() {
      const res = await fetch("/api/tree/");
      const tree = await res.json();
      const root = document.getElementById("fileTree");
      root.innerHTML = '';
      renderTree(tree, root, '');
    }

    function renderTree(node, parent, path) {
      const fullPath = (path === '' ? '' : path) + '/' + node.name;
      const li = document.createElement("li");
      const label = document.createElement("span");
      label.textContent = node.name;

      if (node.type === "directory") {
        li.classList.add("folder");
        li.appendChild(label);
        const ul = document.createElement("ul");
        ul.style.display = "none";
        li.appendChild(ul);

        label.addEventListener("click", function(e) {
          e.stopPropagation();
          const open = ul.style.display === "block";
          ul.style.display = open ? "none" : "block";
          li.classList.toggle("open", !open);
        });

        if (node.children) {
          node.children.forEach(child => renderTree(child, ul, fullPath));
        }
      } else {
        li.classList.add("file");
        li.appendChild(label);
        label.addEventListener("click", () => onFileClick(fullPath));
      }

      parent.appendChild(li);
    }

    function onFileClick(path) {
      if (currentFile && editor.getValue() !== lastContent) {
        const confirmChange = confirm("You have unsaved changes.\n\nPress OK to discard them.\nPress Cancel to keep editing.");
        if (!confirmChange) return;
      }
      loadFile(path);
    }

    function loadFile(path) {
      fetch("/api/file" + path)
        .then(res => {
          if (!res.ok) throw new Error("Load failed");
          return res.text();
        })
        .then(text => {
          currentFile = path;
          lastContent = text;
          document.getElementById("filenameBar").textContent = path;
          try {
            const json = JSON.parse(text);
            editor.session.setMode("ace/mode/json");
            editor.setValue(JSON.stringify(json, null, 2), -1);
          } catch {
            editor.session.setMode("ace/mode/text");
            editor.setValue(text, -1);
          }
        })
        .catch(err => {
          alert("Error loading file: " + err.message);
        });
    }

    function saveFile() {
      if (!currentFile) {
        alert("No file selected.");
        return;
      }

      const content = editor.getValue();

      fetch("/api/file" + currentFile, {
        method: "PUT",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ content: content })
      })
      .then(res => {
        if (res.ok) {
          lastContent = content;
          alert("Saved.");
        } else {
          return res.text().then(text => {
            throw new Error(text);
          });
        }
      })
      .catch(err => {
        alert("Save failed: " + err.message);
      });
    }

    loadTree();
  </script>
</body>
</html>
)rawliteral";

#endif
