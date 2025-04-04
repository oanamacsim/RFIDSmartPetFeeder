#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "MemoryController.h"

// Create an Access Point Server that saves the wifi password into memory
class WebServerController
{
private:
    const char *apSSID = "Feeder_001"; // Access Point SSID
    const char *apPassword = "12345678"; // AP Password (min 8 characters)
    AsyncWebServer* server = NULL; // Web server instance
    MemoryController* memoryController;

    String scannedWiFiAdressesJson;

public:
    WebServerController(MemoryController* memController)
    {
        Serial.println("WebServerController Constructor");
        memoryController = memController;

        updateScannedWiFiAdressesJson();
    }

    ~WebServerController()
    {
        if(server != NULL)
        {
            delete server;
            server = NULL;
        }
    }

    // Start AP mode and web server
    void startAP()
    {
        // start server on port 80
        server = new AsyncWebServer(80);

        // Configure Access Point
        WiFi.softAP(apSSID, apPassword);
        Serial.print("Access Point Started: ");
        Serial.println(apSSID);

        // Setup web server routes
        server->on("/", HTTP_GET, [&](AsyncWebServerRequest *request) {
            handleRoot(request);
        });
        server->on("/scan", HTTP_GET, [&](AsyncWebServerRequest *request) {
            handleScan(request);
        });
        server->on("/saveAdress", HTTP_POST, [&](AsyncWebServerRequest *request) {
            handleSaveWifiAddress(request);
        });

        server->begin();
    }

    void stopAP()
    {
        // Disable the Access Point
        WiFi.softAPdisconnect(true);

        // Stop the web server
        server->end();

        // Print confirmation to Serial Monitor
        Serial.println("Access Point Stopped.");

        delete server;
        server = NULL;
    }

    // Handle root webpage
    void handleRoot(AsyncWebServerRequest *request)
    {
        String html = R"rawliteral(
          <!DOCTYPE html>
          <html>
          <head>
              <style>
                  body {
                      font-family: Arial, sans-serif;
                      margin: 0;
                      padding: 0;
                      display: flex;
                      flex-direction: column;
                      align-items: center;
                      justify-content: center;
                      min-height: 100vh;
                      background-color: #f4f4f9;
                      color: #333;
                  }
                  h2 {
                      margin-bottom: 20px;
                      color: #555;
                  }
                  button, input[type="submit"] {
                      background-color: #4CAF50;
                      color: white;
                      border: none;
                      padding: 10px 20px;
                      text-align: center;
                      text-decoration: none;
                      display: inline-block;
                      font-size: 16px;
                      margin: 10px 5px;
                      cursor: pointer;
                      border-radius: 5px;
                      transition: background-color 0.3s ease;
                  }
                  button:hover, input[type="submit"]:hover {
                      background-color: #45a049;
                  }
                  label {
                      display: block;
                      margin-top: 10px;
                      margin-bottom: 5px;
                      font-weight: bold;
                  }
                  input, select {
                      width: 100%;
                      max-width: 300px;
                      padding: 8px;
                      margin-bottom: 15px;
                      border: 1px solid #ccc;
                      border-radius: 4px;
                      box-sizing: border-box;
                  }
                  #status {
                      margin-top: 10px;
                      font-style: italic;
                      color: #d9534f;
                  }
                  .container {
                      max-width: 400px;
                      padding: 20px;
                      background: white;
                      border: 1px solid #ddd;
                      border-radius: 8px;
                      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
                      text-align: center;
                  }
              </style>
          </head>
          <body>
              <div class="container">
                  <h2>Wi-Fi Configuration</h2>
                  <button onclick="scan()">Scan Networks</button>
                  <select id="ssid" required></select>
                  <label for="password">Password:</label>
                  <input id="password" type="password" placeholder="Enter your Wi-Fi password" required>
                  <input type="submit" value="Save Wi-Fi Address" onclick="saveWifi()">
                  <p id="status"></p>
              </div>
              <script>
                  // Scan for networks
                  function scan() {
                      fetch('/scan')
                      .then(response => response.json())
                      .then(data => {
                          let dropdown = document.getElementById('ssid');
                          dropdown.innerHTML = '';
                          data.forEach(net => {
                              dropdown.innerHTML += `<option value="${net}">${net}</option>`;
                          });
                      });
                  }

                  // Connect to selected network
                  function saveWifi() {
                      let ssid = document.getElementById('ssid').value.trim();
                      let password = document.getElementById('password').value.trim();

                      if (!ssid || !password) {
                          document.getElementById('status').innerText = "Please provide both SSID and password.";
                          return;
                      }

                      let formData = new FormData();
                      formData.append('ssid', ssid);
                      formData.append('password', password);

                      fetch('/saveAdress', {
                          method: 'POST',
                          body: formData
                      })
                      .then(response => {
                          if (!response.ok) {
                              throw new Error(`Error: ${response.status}`);
                          }
                          return response.text();
                      })
                      .then(data => {
                          document.getElementById('status').innerText = data;
                          document.getElementById('status').style.color = '#5cb85c';
                      })
                      .catch(error => {
                          document.getElementById('status').innerText = error.message;
                      });
                  }
              </script>
          </body>
          </html>
          )rawliteral";

        request->send(200, "text/html", html);
    }

    // Handle Wi-Fi scanning
    void handleScan(AsyncWebServerRequest *request)
    {
        request->send(200, "application/json", scannedWiFiAdressesJson);
    }

    void updateScannedWiFiAdressesJson()
    {   
        scannedWiFiAdressesJson = "";

        Serial.println("Scan Wi-Fi networks:");

        WiFi.mode(WIFI_STA);
        WiFi.disconnect(true);
        delay(100);

        Serial.println("Starting asynchronous Wi-Fi scan...");
        int scanStart = WiFi.scanNetworks(true); // Start asynchronous scan
        if (scanStart == WIFI_SCAN_RUNNING) 
        {
            Serial.println("Scan started successfully!");
        }
        else
        {
            Serial.printf("Failed to start scan. Error code: %d\n", scanStart);
            scannedWiFiAdressesJson = "[]";
            return;
        }

        // Wait for scan to complete (polling)
        int timeout = 10; // 10 seconds timeout
        while (WiFi.scanComplete() == WIFI_SCAN_RUNNING && timeout > 0) 
        {
            delay(1000); // Wait 1 second
            timeout--;
            Serial.print(".");
        }

        int n = WiFi.scanComplete(); // Get the scan result
        scannedWiFiAdressesJson = "[";

        if (n <= 0) 
        {
            if (n == WIFI_SCAN_FAILED) 
            {
                Serial.println("Wi-Fi scan failed.");
            }
            else
            {
                Serial.println("No networks found.");
            }    
        } 
        else
        {
            Serial.printf("\nFound %d networks:\n", n);
            for (int i = 0; i < n; ++i) 
            {
                if (i > 0)
                    scannedWiFiAdressesJson += ",";
                scannedWiFiAdressesJson += "\"" + WiFi.SSID(i) + "\"";

                Serial.printf("%d: %s (%d dBm) %s\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i), (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "encrypted");
            }
            WiFi.scanDelete(); // Clear scan results
        }

        scannedWiFiAdressesJson += "]";

        Serial.println("Scan complete, JSON returned.");
    }


    // Handle Wi-Fi save address
    void handleSaveWifiAddress(AsyncWebServerRequest *request) 
    {
        String ssid;
        String password;

        // Extract request parameters
        for (int i = 0; i < request->params(); i++) 
        {
            const AsyncWebParameter* p = request->getParam(i);
            if (p->name() == "ssid") 
            {
                ssid = p->value();
            }
            else if (p->name() == "password") 
            {
                password = p->value();
            }

            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }

        // Check if both SSID and password were provided
        if (ssid.isEmpty() || password.isEmpty()) 
        {
            request->send(400, "text/plain", "Missing SSID or Password.");
            return;
        }

        // Save Wi-Fi credentials
        memoryController->saveWifiData(ssid, password);

        // Respond with success
        request->send(200, "text/plain", "Wi-Fi network saved successfully!");

        delay(3000);

        ESP.restart(); // restart to start reconnecting to the new set wifi password
    }
};