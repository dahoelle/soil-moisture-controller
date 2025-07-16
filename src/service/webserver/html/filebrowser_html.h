#ifndef FILEBROWSER_HTML_H
#define FILEBROWSER_HTML_H

#include <Arduino.h>

const char FILEBROWSER_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>File Browser</title>
  <style>
    body {
      margin: 0;
      font-family: sans-serif;
      background-color: #1a1a1a;
      color: #ccc;
      display: flex;
      flex-direction: column;
      height: 100vh;
      overflow: hidden;
    }

    header {
      background: #222;
      padding: 1rem;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    header h2 {
      margin: 0;
      font-size: 1.2rem;
    }

    button {
      background: #444;
      color: #eee;
      border: none;
      padding: 0.5rem 1rem;
      cursor: pointer;
    }

    #fileTree {
      list-style-type: none;
      padding-left: 1rem;
      margin: 0;
      overflow-y: auto;
      flex: 1;
      font-family: monospace;
      font-size: 14px;
    }

    li span:hover {
        color: #fff;
        text-decoration: underline;
    }

    .folder::before {
      content: '▶';
      display: inline-block;
      margin-right: 5px;
    }

    .folder.open::before {
      content: '▼';
    }

    .file {
      padding-left: 1.2em;
    }

    .file:hover, .folder:hover {
      color: #fff;
      text-decoration: underline;
    }

    main {
      padding: 1rem;
      overflow-y: auto;
      flex: 1;
    }
  </style>
</head>
<body>
  <header>
    <h2>File Browser</h2>
    <button onclick="loadTree()">Reload</button>
  </header>
  <main>
    <ul id="fileTree"></ul>
  </main>

  <script>
    async function loadTree() {
      try {
        const res = await fetch('/api/tree/');
        const tree = await res.json();
        const root = document.getElementById('fileTree');
        root.innerHTML = '';
        renderTree(tree, root, '');
      } catch (e) {
        console.error("Failed to load file tree:", e);
      }
    }

    function renderTree(node, parent, currentPath) {
        if (!node) return;

        const li = document.createElement('li');
        const fullPath = currentPath + '/' + node.name;

        const label = document.createElement('span');
        label.textContent = node.name;

        if (node.type === 'directory') {
            li.classList.add('folder');
            const ul = document.createElement('ul');
            ul.style.display = 'none';
            li.appendChild(label);
            li.appendChild(ul);

            label.addEventListener('click', function(e) {
            e.stopPropagation();
            const open = ul.style.display === 'block';
            ul.style.display = open ? 'none' : 'block';
            li.classList.toggle('open', !open);
            });

            parent.appendChild(li);
            if (node.children) {
            node.children.forEach(child => renderTree(child, ul, fullPath));
            }
        } else {
            li.classList.add('file');
            li.appendChild(label);
            label.addEventListener('click', function(e) {
            e.stopPropagation();
            console.log(fullPath);
            });
            parent.appendChild(li);
        }
        }


    loadTree();
  </script>
</body>
</html>
)rawliteral";

#endif
