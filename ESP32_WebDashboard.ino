#include <WiFi.h>
#include <WebServer.h>
#include "time.h"

const char* ssid = "SSID"; // Add your WiFi SSID
const char* password = "Password"; // Enter your Wifi Password

const int ledPin = 2; // Built-in LED on most ESP32 boards
WebServer server(80);

bool ledState = false;

// Configure the NTP time server to get the time
const char* ntpServer = "pool.ntp.org"; 
const long gmtOffset_sec = 3600; // Adjust this to your timezone offset (e.g., 3600 for UTC+1)
const int daylightOffset_sec = 3600; // Adjust this if daylight savings applies

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Control</title>
  <style>
    button {
      padding: 20px;
      font-size: 20px;
      cursor: pointer;
    }
  </style>
</head>
<body>
  <h1>ESP32 Control</h1>
  <p>LED Status: <span id="ledStatus">OFF</span></p>
  <button onclick="toggleLED()">Toggle LED</button>
  <h2>Real-Time Values</h2>
  <p>Temperature: <span id="temperature"></span> &deg;C</p>
  <p>Raw Temperature: <span id="rawTemperature"></span></p>
  <p>Voltage: <span id="voltage"></span> V</p>
  <p>RTC Time: <span id="rtcTime"></span></p>

  <script>
    function updateLEDStatus() {
      fetch('/status')
        .then(response => response.text())
        .then(status => {
          document.getElementById('ledStatus').textContent = status;
        });
    }

    function updateRealTimeValues() {
      fetch('/getTemperature')
        .then(response => response.text())
        .then(temp => {
          document.getElementById('temperature').textContent = temp + " °C";
        });

      fetch('/getRawTemperature')
        .then(response => response.text())
        .then(rawTemp => {
          document.getElementById('rawTemperature').textContent = rawTemp;
        });

      fetch('/getVoltage')
        .then(response => response.text())
        .then(voltage => {
          document.getElementById('voltage').textContent = voltage;
        });

      fetch('/getRTC')
        .then(response => response.text())
        .then(rtc => {
          document.getElementById('rtcTime').textContent = rtc;
        });
    }

    function toggleLED() {
      fetch('/toggle')
        .then(() => updateLEDStatus());
    }

    // Function to update real-time data every 3 seconds (adjust the interval as needed)
    setInterval(updateRealTimeValues, 3000);  // Update every 3 seconds

    // Function to update LED status on page load
    updateLEDStatus();
    // Initial update for real-time values
    updateRealTimeValues();
  </script>
</body>
</html>
)rawliteral";


// Handle root URL to serve the HTML page
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// Handle /toggle URL to toggle LED and return its new state
void handleToggle() {
  ledState = !ledState;
  digitalWrite(ledPin, ledState ? HIGH : LOW);
  server.send(200, "text/plain", ledState ? "ON" : "OFF");
}

// Handle /status URL to return the current LED state
void handleStatus() {
  server.send(200, "text/plain", ledState ? "ON" : "OFF");
}

// Function to get internal temperature from ESP32
float getInternalTemperature() {
  int tempRaw = temperatureRead();  // Get raw temperature in Celsius
  float tempCelsius = (tempRaw - 32) / 1.8;  // Convert from Fahrenheit to Celsius
  tempCelsius -= 27; // Apply offset correction (you might need to adjust this)
  return tempCelsius;  // Return corrected temperature
}

// Function to get the ESP32's supply voltage
float getVoltage() {
  int adcValue = analogRead(34); // ADC pin 34 for voltage measurement
  return (adcValue / 4095.0) * 3.3;  // Convert ADC value to voltage
}

// Function to get the RTC time from the ESP32
String getRTC() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Failed to get time";
  }
  char timeStr[20];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(timeStr);
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); // Start with LED off

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  // Set up NTP for RTC
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.on("/status", handleStatus); // Route to get LED status
  server.on("/getTemperature", []() {
    server.send(200, "text/plain", String(getInternalTemperature()));
  });
  server.on("/getVoltage", []() {
    server.send(200, "text/plain", String(getVoltage()));
  });
  server.on("/getRTC", []() {
    server.send(200, "text/plain", getRTC());
  });

  server.begin();
  Serial.println("Web server started!");
}

void loop() {
  server.handleClient();
}
