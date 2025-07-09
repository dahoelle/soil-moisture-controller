#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>

// Store HTML page in PROGMEM to save RAM
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8" />
    <title>ESP32 Web Server</title>
</head>
<body>
    <h1>ESP32 Web Server</h1>
    <p>Welcome to the WebServerService!</p>
</body>
</html>
)rawliteral";

#endif
