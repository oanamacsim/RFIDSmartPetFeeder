#ifndef MEMORY_CONTROLLER_H
#define MEMORY_CONTROLLER_H

#include <Preferences.h>
#include "FeederDataTypes.h"

class MemoryController
{
private:
    // Use Preferences for NVS storage
    Preferences preferences; 

    // Constants for NVS keys
    static constexpr const char* NVS_NAMESPACE = "feeder";
    static constexpr const char* KEY_WIFI_SSID = "wifiSSID";
    static constexpr const char* KEY_WIFI_PASSWORD = "wifiPassword";
    static constexpr const char* KEY_FOOD_CONFIG = "foodConfig";
    static constexpr const char* KEY_TRAP_MODE = "trapMode";
    static constexpr const char* KEY_ID = "id";
    static constexpr const char* KEY_NAME = "name";
    static constexpr const char* KEY_FOOD_STORAGE_QUANTITY = "foodStorageQuantity";
    static constexpr const char* KEY_FOOD_CURRENT_WEIGHT = "foodCurrentWeight";
    static constexpr const char* KEY_LAST_FOOD_STORAGE_UPDATE_TIME = "lastFoodStorageUpdateTime";
    static constexpr const char* KEY_LAST_FOOD_WEIGHT_UPDATE_TIME = "lastFoodCurrentWeightUpdateTime";
    static constexpr const char* KEY_WEIGHT_TO_LOAD_AT_RESTART = "weightToLoadAtRestart";

    void beginPreferences(bool readOnly)
    {
        preferences.begin(NVS_NAMESPACE, readOnly);
    }

    void endPreferences()
    {
        preferences.end();
    }

public:
    const String feederId = "feeder_001";
    const String feederPassword = "parola1234";

    MemoryController() = default;

    void saveWifiData(const String& ssid, const String& password)
    {
        Serial.println("MemoryController::Wifi saved");

        beginPreferences(false); // Open NVS in write mode
        preferences.putString(KEY_WIFI_SSID, ssid);
        preferences.putString(KEY_WIFI_PASSWORD, password);
        endPreferences();
    }

    void saveFeederConfiguration(const String& foodConfigurationJson, const String& trapMode, const String& id, const String& name, float foodStorageQuantity, float foodCurrentWeight, unsigned long lastFoodStorageQuantityUpdateTime, unsigned long lastFoodCurrentWeightUpdateTime)
    {
        Serial.println("MemoryController::food config saved");

        beginPreferences(false); // Open NVS in write mode

        preferences.putString(KEY_FOOD_CONFIG, foodConfigurationJson);
        preferences.putString(KEY_TRAP_MODE, trapMode);
        preferences.putString(KEY_ID, id);
        preferences.putString(KEY_NAME, name);
        preferences.putFloat(KEY_FOOD_STORAGE_QUANTITY, foodStorageQuantity);
        preferences.putFloat(KEY_FOOD_CURRENT_WEIGHT, foodCurrentWeight);
        preferences.putULong(KEY_LAST_FOOD_STORAGE_UPDATE_TIME, lastFoodStorageQuantityUpdateTime);
        preferences.putULong(KEY_LAST_FOOD_WEIGHT_UPDATE_TIME, lastFoodCurrentWeightUpdateTime);

        endPreferences();

        // Log the saved values for debugging
        Serial.println("Configuration saved to memory:");
        Serial.println("Food Config JSON: " + foodConfigurationJson);
        Serial.println("Trap Mode: " + trapMode);
        Serial.println("ID: " + id);
        Serial.println("Name: " + name);
        Serial.println("Food Storage Quantity: " + String(foodStorageQuantity));
        Serial.println("Food Current Weight: " + String(foodCurrentWeight));
        Serial.println("Last Food Storage Update Time: " + String(lastFoodStorageQuantityUpdateTime));
        Serial.println("Last Food Current Weight Update Time: " + String(lastFoodCurrentWeightUpdateTime));
    }

    void setWeightToLoadAtRestart(int weight)
    {
        beginPreferences(false); // Open in read-write mode
        preferences.putInt(KEY_WEIGHT_TO_LOAD_AT_RESTART, weight);
        endPreferences();
    }

    int getWeightToLoadAtRestart()
    {
        beginPreferences(false); // Open in read-write mode
        int weight = preferences.getInt(KEY_WEIGHT_TO_LOAD_AT_RESTART, 0);
        preferences.putInt(KEY_WEIGHT_TO_LOAD_AT_RESTART, 0);
        endPreferences();
        return weight;
    }

    String getFeederWifiSSID()
    {
        beginPreferences(true); // Open NVS in read-only mode
        String ssid = preferences.getString(KEY_WIFI_SSID, "");
        endPreferences();
        return ssid;
    }

    String getFeederWiFiPassword()
    {
        beginPreferences(true); // Open NVS in read-only mode
        String password = preferences.getString(KEY_WIFI_PASSWORD, "");
        endPreferences();
        return password;
    }

    String getFoodConfigJson()
    {
        beginPreferences(true); // Open NVS in read-only mode
        String feedConfig = preferences.getString(KEY_FOOD_CONFIG, "");
        endPreferences();
        return feedConfig;
    }
};

#endif // MEMORY_CONTROLLER_H