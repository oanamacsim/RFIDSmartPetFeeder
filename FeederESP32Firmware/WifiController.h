#include <WebServerController.h>
#include <WebConnectionController.h>
#include "MemoryController.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class WifiController
{
private:
    MemoryController* memoryController;
    WebServerController* webServer = nullptr;
    WebConnectionController* webConnection = nullptr;

    int retryCount = 0;
    const int maxRetries = 3;

    int serverStartTime = 0;
    const int serverTimeout = 180000; // 5-minute
    bool isServerActive = false;

    void startWebServer()
    {
        webServer->startAP();
    }

    void stopWebServer()
    {
        webServer->stopAP();
    }

    void startWifiClient()
    { 
        webConnection->connectToWifi();
    }

    void stopWifiClient()
    {
        webConnection->disconnect();
    }

public:
    WifiController(MemoryController* memController)
    {
        Serial.println("WifiController Constructor...");
        memoryController = memController;
        webServer = new WebServerController(memoryController);
        webConnection = new WebConnectionController(memoryController);
        WiFi.mode(WIFI_AP_STA);
        startWifiClient();
    }

    WebConnectionController* getWebConnection()
    {
        return webConnection;
    }

    void loop()
    {   
        if (!isServerActive && !webConnection->haveInternetConnection())
        {
            if (retryCount < maxRetries)
            {
                Serial.println("WiFi connection try" +  String(retryCount) + "failed, retrying...");
                retryCount++;
                startWifiClient();
                delay(1000);
            }
            else
            {
                Serial.println("Max retries reached, starting WebServer AP for 2 minutes...");
                stopWifiClient();
                startWebServer();
                isServerActive = true;
                serverStartTime = millis();
                retryCount = 0; // Reset retry count
            }
        }

        if (isServerActive && millis() - serverStartTime > serverTimeout)
        {   
            Serial.println("WebServer AP timeout, stopping AP and retrying WiFi connection...");
            stopWebServer();
            isServerActive = false;
            startWifiClient();
            retryCount = 0;
        }
    }
};

