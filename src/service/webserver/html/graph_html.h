#ifndef GRAPH_HTML_H
#define GRAPH_HTML_H

#include <Arduino.h>

const char GRAPH_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8" />
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>Sensor Graph - Dark Mode</title>
<style>
  html, body {
    margin: 0; padding: 0; font-family: sans-serif;
    background-color: #121212;
    color: #e0e0e0;
  }
  canvas {
    width: 100%;
    height: 90vh;
    display: block;
    background-color: #1e1e1e;
    border: 1px solid #333;
    border-radius: 4px;
  }
  #controls {
    padding: 1rem;
    background-color: #1f1f1f;
    border-bottom: 1px solid #333;
  }
  input, button {
    font-size: 1rem;
    background-color: #333;
    color: #e0e0e0;
    border: 1px solid #555;
    border-radius: 3px;
    padding: 0.3rem 0.5rem;
  }
  input::placeholder {
    color: #888;
  }
  input:focus, button:hover {
    outline: none;
    border-color: #66aaff;
    box-shadow: 0 0 5px #66aaff;
  }
</style>
</head>
<body>
<div id="controls">
  <input type="text" id="fileInput" placeholder="/logs/20250713_14.json" size="40" />
  <button onclick="loadFile()">Load</button>
</div>
<canvas id="chart"></canvas>

<script src="https://cdn.jsdelivr.net/npm/chart.js@4.3.0/dist/chart.umd.min.js"></script>
<script src="https://cdn.jsdelivr.net/npm/chartjs-adapter-date-fns@2.0.0/dist/chartjs-adapter-date-fns.bundle.min.js"></script>

<script>
  let chart;

  window.addEventListener('DOMContentLoaded', () => {
    const ctx = document.getElementById('chart').getContext('2d');
    chart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: [],
        datasets: [
          { label: 'ADC4', borderColor: 'red', data: [], fill: false },
          { label: 'ADC5', borderColor: 'blue', data: [], fill: false },
          { label: 'ADC6', borderColor: 'green', data: [], fill: false }
        ]
      },
      options: {
        responsive: true,
        scales: {
          x: {
            type: 'time',
            time: { unit: 'minute' },
            title: { display: true, text: 'Time', color: '#e0e0e0' },
            ticks: { color: '#bbb' },
            grid: { color: '#333' }
          },
          y: {
            beginAtZero: true,
            title: { display: true, text: 'ADC Value', color: '#e0e0e0' },
            ticks: { color: '#bbb' },
            grid: { color: '#333' }
          }
        },
        plugins: {
          legend: { position: 'top', labels: { color: '#e0e0e0' } },
          title: { display: true, text: 'Sensor Readings (GPIO4–6)', color: '#e0e0e0' }
        }
      }
    });
  });

  function loadFile() {
    if (!chart) {
      alert('Chart not initialized yet.');
      return;
    }

    const path = document.getElementById('fileInput').value.trim();
    if (!path) {
      alert("Enter a valid file path.");
      return;
    }

    fetch("/api/file" + path)
      .then(res => {
        if (!res.ok) throw new Error("Failed to load file");
        return res.text();
      })
      .then(text => {
        const lines = text.trim().split("\n");
        const labels = [], adc4 = [], adc5 = [], adc6 = [];

        for (let line of lines) {
          try {
            const obj = JSON.parse(line);
            if (!obj.timestamp) continue;
            labels.push(new Date(obj.timestamp * 1000));
            adc4.push(obj.ADC4 ?? null);
            adc5.push(obj.ADC5 ?? null);
            adc6.push(obj.ADC6 ?? null);
          } catch (e) {
            console.warn("Skipping invalid JSON line:", line);
          }
        }

        chart.data.labels = labels;
        chart.data.datasets[0].data = adc4;
        chart.data.datasets[1].data = adc5;
        chart.data.datasets[2].data = adc6;
        chart.update();
      })
      .catch(err => {
        alert("Error loading file: " + err.message);
      });
  }
</script>
</body>
</html>
)rawliteral";

#endif
