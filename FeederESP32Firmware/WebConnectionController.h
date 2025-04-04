#ifndef WEB_CONNECTION_CONTROLLER_H
#define WEB_CONNECTION_CONTROLLER_H

#include <HTTPClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <MemoryController.h>

class WebConnectionController
{
private:
    String wifiSSID;
    String wifiPassword;

    String FeederId;
    String FeederPassword;

    unsigned long syncedTimestamp = 0; // The Unix time when the last sync occurred
    unsigned long syncMillis = 0;      // The millis value at the time of the last sync

    MemoryController* memoryController = nullptr;

public:
    // Constructor to initialize Wi-Fi credentials
    WebConnectionController(MemoryController* memController)
    {
        Serial.println("WebConnectionController Constructor");

        memoryController = memController;

        wifiSSID = memoryController->getFeederWifiSSID();
        wifiPassword = memoryController->getFeederWiFiPassword();
        FeederId = memoryController->feederId;
        FeederPassword = memoryController->feederPassword;

        Serial.println("WebConnectionController Initialized");
    }

    static int getRelativeMinutesSinceMidnight(unsigned long unixTime) 
    {
      return (unixTime % 86400L) / 60; // Extracts hours and minutes as total minutes since midnight
    }

    static String debugMinutes(unsigned long minutesSinceMidnight) 
    {
        int hour = minutesSinceMidnight / 60;
        int minutes = minutesSinceMidnight % 60;

        return String(hour) + ":" + String(minutes);
    }

    // Connect to Wi-Fi
    bool connectToWifi()
    {
        WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

        Serial.println("Connect to wifi begin: ");
        Serial.println("Ssid: " + wifiSSID);
        Serial.println("Password: " + wifiPassword);

        int maxRetries = 10; // Maximum number of retries to connect
        int retryCount = 0;

        // Wait for connection
        while (WiFi.status() != WL_CONNECTED && retryCount < maxRetries)
        {
            delay(750);
            Serial.print(".");
            retryCount++;
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("\nWi-Fi connected!");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());

            delay(1000);
            //synchronizeTime();

            fetchFeederData();

            //addFoodDispenseEvent(getCurrentTime(), 0.35f);
            //addGateEvent(getCurrentTime(), getCurrentTime() + 100);
            return true;
        }
        else
        {
            Serial.println("\nFailed to connect to Wi-Fi");
            disconnect(); // to reset wifi module
            return false;
        }
    }

    bool haveInternetConnection()
    {
        return WiFi.status() == WL_CONNECTED;
    }

    // Perform an HTTP GET request
    String httpGetRequest(const String &url)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("Wi-Fi not connected!");
            return "";
        }

        HTTPClient http;
        http.begin(url);

        int httpResponseCode = http.GET();

        String payload = "";
        if (httpResponseCode > 0)
        {
            payload = http.getString();
        }
        else
        {
            Serial.print("Error on HTTP request: ");
            Serial.println(httpResponseCode);
        }

        http.end();
        return payload;
    }

    // Perform an HTTP POST request
    String httpPostRequest(const String &url, const String &payload, const String &contentType = "application/json")
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("Wi-Fi not connected!");
            return "";
        }

        HTTPClient http;
        http.begin(url);
        http.addHeader("Content-Type", contentType);

        int httpResponseCode = http.POST(payload);

        String response = "";
        if (httpResponseCode > 0)
        {
            response = http.getString();
        }
        else
        {
            Serial.print("Error on HTTP POST request: ");
            Serial.println(httpResponseCode);
        }

        http.end();
        return response;
    }

    String httpPutRequest(const String &url, const String &payload, const String &contentType = "application/json")
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("Wi-Fi not connected!");
            return "";
        }

        HTTPClient http;
        http.begin(url);
        http.addHeader("Content-Type", contentType);

        int httpResponseCode = http.PUT(payload);

        String response = "";
        if (httpResponseCode > 0)
        {
            response = http.getString();
        }
        else
        {
            Serial.print("Error on HTTP PUT request: ");
            Serial.println(httpResponseCode);
        }

        http.end();
        return response;
    }

    bool addGateEvent(int startTime, int endTime)
    {
        Serial.println("AddGateEvent");
        if (!haveInternetConnection())
        {
            Serial.println("No internet connection. Cannot add gate event.");
            return false;
        }

        if(startTime == 0 || endTime == 0)
        {
            Serial.println("Cannot addGateEvent. Invalid Time");
            return false;
        }

        // Define the API endpoint
        const String apiUrl = "https://dev.bull-software.com/add_gate_event.php";

        StaticJsonDocument<256> jsonDoc;
        jsonDoc["ID"] = FeederId;
        jsonDoc["Password"] = FeederPassword;
        jsonDoc["startTime"] = startTime;
        jsonDoc["endTime"] = endTime;

        String jsonPayload;
        serializeJson(jsonDoc, jsonPayload);

        // Perform HTTP POST request
        Serial.println("Sending HTTP POST request to add gate event...");
        String response = httpPostRequest(apiUrl, jsonPayload, "application/json");

        // Handle the response
        if (response.isEmpty())
        {
            Serial.println("Failed to get a response from the server.");
            return false;
        }

        Serial.print("Server response: ");
        Serial.println(response);
        return true;
    }

    bool addFoodDispenseEvent(unsigned long dispensedAt, float quantityDispensed)
    {
        Serial.println("AddFoodDispanseEvent");
        if (!haveInternetConnection())
        {
            Serial.println("No internet connection. Cannot add food dispense event.");
            return false;
        }

        if(dispensedAt == 0)
        {
            Serial.println("Cannot AddFoodDispanseEvent. Invalid Time");
            return false;
        }

        // Define the API endpoint
        const String apiUrl = "https://dev.bull-software.com/add_food_dispense_event.php";

        StaticJsonDocument<256> jsonDoc;
        jsonDoc["ID"] = FeederId;
        jsonDoc["Password"] = FeederPassword;
        jsonDoc["dispensedAt"] = dispensedAt;
        jsonDoc["quantityDispensed"] = quantityDispensed;

        String jsonPayload;
        serializeJson(jsonDoc, jsonPayload);

        // Perform HTTP POST request
        Serial.println("Sending HTTP POST request to add food dispense event...");
        String response = httpPostRequest(apiUrl, jsonPayload, "application/json");

        // Handle the response
        if (response.isEmpty())
        {
            Serial.println("Failed to get a response from the server.");
            return false;
        }

        Serial.print("Server response: ");
        Serial.println(response);
        return true;
    }

    void updateFoodWeight(float newWeight, int currentTime)
    {
        Serial.println("Updating food weight...");
        
        if (!haveInternetConnection())
        {
            Serial.println("No internet connection. Cannot update food weight.");
            return;
        }

        if(currentTime == 0)
        {
             Serial.println("updateFoodWeight Cannot be updated because time was not synched yet");
        }
        
        // Define the API endpoint
        const String apiUrl = "https://dev.bull-software.com/update_food_weight.php";
        
        // Create JSON payload
        StaticJsonDocument<256> jsonDoc;
        jsonDoc["ID"] = FeederId;
        jsonDoc["Password"] = FeederPassword;
        jsonDoc["FoodCurrentWeight"] = newWeight;
        jsonDoc["LastFoodCurrentWeightUpdateTime"] = currentTime;
        
        String jsonPayload;
        serializeJson(jsonDoc, jsonPayload);
        
        // Perform HTTP PUT request
        Serial.println("Sending HTTP PUT request to update food weight...");
        String response = httpPutRequest(apiUrl, jsonPayload, "application/json");
        
        // Handle the response
        if (response.isEmpty())
        {
            Serial.println("Failed to get a response from the server.");
            return;
        }
        
        Serial.print("Server response: ");
        Serial.println(response);
    }

    void fetchFeederData()
    {
        // Construct the full API URL with query parameters
        String url = "https://dev.bull-software.com/get_feeder.php?ID=" + FeederId + "&Password=" + FeederPassword;

        Serial.println("Sending GET request to: " + url);
        String response = httpGetRequest(url);

        if (response.length() > 0)
        {
            Serial.println("Response received:");
            Serial.println(response);

            // Parse the JSON response
            StaticJsonDocument<1024> doc; // Adjust size if necessary
            DeserializationError error = deserializeJson(doc, response);

            if (error)
            {
                Serial.print("JSON parsing failed: ");
                Serial.println(error.c_str());
                return;
            }

            // Extract fields from JSON response
            if (doc.containsKey("error"))
            {
                Serial.print("API Error: ");
                Serial.println(doc["error"].as<String>());
                return;
            }

            String id = doc["ID"].as<String>();
            String name = doc["Name"].as<String>();
            String trapMode = doc["TrapMode"].as<String>();
            String feedFoodConfigJson = doc["FeedFoodConfiguration"].as<String>();
            float foodStorageQuantity = doc["FoodStorageQuantity"].as<float>();
            float foodCurrentWeight = doc["FoodCurrentWeight"].as<float>();
            unsigned long lastFoodStorageQuantityUpdateTime = doc["LastFoodStorageQuantityUpdateTime"].as<unsigned long>();
            unsigned long lastFoodCurrentWeightUpdateTime = doc["LastFoodCurrentWeightUpdateTime"].as<unsigned long>();

            Serial.println("Feeder Data:");
            Serial.println("ID: " + id);
            Serial.println("Name: " + name);
            Serial.println("Trap Mode: " + trapMode);
            Serial.println("Food Storage Quantity: " + String(foodStorageQuantity));
            Serial.println("Food Current Weight: " + String(foodCurrentWeight));
            Serial.println("Last Update lastFoodStorageQuantityUpdateTime: " + String(lastFoodStorageQuantityUpdateTime));
            Serial.println("Last Update lastFoodCurrentWeightUpdateTime: " + String(lastFoodCurrentWeightUpdateTime));

            memoryController->saveFeederConfiguration(feedFoodConfigJson, trapMode, id, name, foodStorageQuantity, foodCurrentWeight, lastFoodStorageQuantityUpdateTime, lastFoodCurrentWeightUpdateTime);
        }
        else
        {
            Serial.println("Failed to get a response from the API.");
        }
    }

    String getCommandFromApplication()
    {
        if (!haveInternetConnection())
        {
            Serial.println("No internet connection. Cannot fetch command.");
            return "";
        }

        const String apiUrl = "https://dev.bull-software.com/get_esp32_command.php?ID=" + FeederId + "&Password=" + FeederPassword;

        HTTPClient http;
        http.begin(apiUrl);
        int httpResponseCode = http.GET();

        String response = "";
        if (httpResponseCode > 0)
        {
            response = http.getString();
            //Serial.println("Command Response: " + response);

            if(response.length() > 0)
            {
                StaticJsonDocument<1024> doc;
                DeserializationError error = deserializeJson(doc, response);

                if (error)
                {
                    Serial.print("JSON parsing failed: ");
                    Serial.println(error.c_str());
                    http.end();
                    return "";
                }

                if (doc.containsKey("error") || doc.containsKey("message"))
                {
                    Serial.print("API Error: ");
                    Serial.println(doc["error"].as<String>());
                    http.end();
                    return "";
                }

                String command = doc["Command"].as<String>();
                response = command;
            }
            else
            {
                response = "";
            }
        }
        else
        {
            Serial.print("Error on HTTP GET request: ");
            Serial.println(httpResponseCode);
            response = "";
        }

        http.end();
        return response;
    }

    bool synchronizeTime() 
    {
        if (WiFi.status() != WL_CONNECTED) 
        {
            Serial.println("Wi-Fi not connected! Cannot sync time.");
            return false;
        }

        int numOfTries = 5;

        for(int tryNumber = 1; tryNumber <= numOfTries; tryNumber++)
        {
            Serial.println("Trying to sync time. Try number:" + tryNumber);
            const char* apis[] = 
            {
                "http://worldtimeapi.org/api/timezone/Etc/UTC",
                "http://worldclockapi.com/api/json/utc/now",
                "https://timeapi.io/api/Time/current/zone?timeZone=UTC"
            };

            for (int i = 0; i < 3; i++) 
            {
                HTTPClient http;
                http.begin(apis[i]);
                Serial.print("Trying getting time with API: ");
                Serial.println(apis[i]);

                int httpResponseCode = http.GET();

                if (httpResponseCode > 0) 
                {
                    String response = http.getString();
                    Serial.println("Time sync response: " + response);

                    // Extract the UNIX timestamp from the JSON response
                    int timeIndex;
                    if (apis[i] == apis[0]) 
                    {
                        timeIndex = response.indexOf("\"unixtime\":");
                        if (timeIndex != -1) 
                        {
                            syncedTimestamp = response.substring(timeIndex + 11).toInt();
                        }
                    } 
                    else if (apis[i] == apis[1]) 
                    {
                        timeIndex = response.indexOf("\"currentFileTime\":");
                        if (timeIndex != -1) 
                        {
                            // Extract the "currentFileTime" value
                            String fileTimeStr = response.substring(timeIndex + 18, response.indexOf(",", timeIndex));
                            
                            // Convert the extracted string to a 64-bit integer
                            uint64_t fileTime = strtoull(fileTimeStr.c_str(), NULL, 10);
                            // (133815211641693715 / 10000000) - 11644473600;
                            // Convert from 100-nanoseconds since 1601 to UNIX time
                            syncedTimestamp = (fileTime / 10000000) - 11644473600; //GMT+0000
                        }
                    }
                    else if (apis[i] == apis[2]) 
                    {
                        // Parse the JSON response
                        int yearIndex = response.indexOf("\"year\":");
                        int monthIndex = response.indexOf("\"month\":");
                        int dayIndex = response.indexOf("\"day\":");
                        int hourIndex = response.indexOf("\"hour\":");
                        int minuteIndex = response.indexOf("\"minute\":");
                        int secondsIndex = response.indexOf("\"seconds\":");

                        if (yearIndex != -1 && monthIndex != -1 && dayIndex != -1 &&
                            hourIndex != -1 && minuteIndex != -1 && secondsIndex != -1) 
                        {
                            int year = response.substring(yearIndex + 7, response.indexOf(",", yearIndex)).toInt();
                            int month = response.substring(monthIndex + 8, response.indexOf(",", monthIndex)).toInt();
                            int day = response.substring(dayIndex + 6, response.indexOf(",", dayIndex)).toInt();
                            int hour = response.substring(hourIndex + 7, response.indexOf(",", hourIndex)).toInt();
                            int minute = response.substring(minuteIndex + 9, response.indexOf(",", minuteIndex)).toInt();
                            int seconds = response.substring(secondsIndex + 10, response.indexOf(",", secondsIndex)).toInt();

                            // Convert to UNIX time
                            struct tm timeinfo;
                            timeinfo.tm_year = year - 1900;  // tm_year is years since 1900
                            timeinfo.tm_mon = month - 1;    // tm_mon is 0-based
                            timeinfo.tm_mday = day;
                            timeinfo.tm_hour = hour;
                            timeinfo.tm_min = minute;
                            timeinfo.tm_sec = seconds;

                            // Use mktime to compute the UNIX timestamp
                            time_t rawTime = mktime(&timeinfo); // GMT+0000 timezone
                            if (rawTime != -1) 
                            {
                                syncedTimestamp = (uint32_t)rawTime;
                                syncMillis = millis();
                                Serial.print("Synced Timestamp from timeapi.io: ");
                                Serial.println(syncedTimestamp);
                                return true;
                            }
                        }
                    }

                    if (syncedTimestamp > 0) 
                    {
                        syncMillis = millis(); // Record when the sync happened
                        Serial.print("Time Synced Succesfull: Timestamp: "); // + syncMillis
                        Serial.println(String(syncedTimestamp));
                        http.end();
                        return true;
                    }
                } 
                else 
                {
                    Serial.print("HTTP GET failed with code: ");
                    Serial.println(httpResponseCode);
                }

                http.end();
            }
        }

        Serial.println("Failed to sync time with all APIs.");
        return false;
    }

    // Function to get the current real-time (non-blocking)
    unsigned long getCurrentTime(bool ro_time = false) 
    {
        if (syncedTimestamp == 0) 
        {
            Serial.println("Time not synced yet!");
            return 0; // Return 0 if time is not synced
        }

        unsigned long gmtPlusTwoOffset = 0;

        if(ro_time)
        {
            gmtPlusTwoOffset = 7200;
        }

        // Calculate the current time based on syncedTimestamp and elapsed time
        return syncedTimestamp + ((millis() - syncMillis) / 1000) + gmtPlusTwoOffset;
    }

    void disconnect()
    {
        WiFi.disconnect();
    }
};

#endif // WEB_CONNECTION_CONTROLLER_H