#ifndef FEEDER_CONFIG_H
#define FEEDER_CONFIG_H

#include <ArduinoJson.h>

struct FeedConfigEntry
{
    String dispenseTime; // Time in "HH:MM" format
    float quantity;      // Quantity of food to dispense
    bool wasDispensedToday = false; // Flag to track if the entry was dispensed today

    FeedConfigEntry& operator=(const FeedConfigEntry& other)
    {
        // Prevent self-assignment
        if (this != &other) 
        {
            dispenseTime = other.dispenseTime;
            quantity = other.quantity;
            wasDispensedToday = other.wasDispensedToday;
        }

        // Return reference to allow chained assignment
        return *this;
    }

    String toString() const
    {
        return "Dispense Time: " + dispenseTime + ", Quantity: " + String(quantity);
    }

    int getHours() const
    {
        int colonIndex = dispenseTime.indexOf(':');
        if (colonIndex == -1 || colonIndex == 0 || colonIndex >= dispenseTime.length() - 1)
        {
            return -1; // Invalid format
        }

        String hourStr = dispenseTime.substring(0, colonIndex);
        int hour = hourStr.toInt();

        if (hour < 0 || hour > 23)
        {
            return -1; // Invalid hour
        }

        return hour;
    }

    int getMinutes() const
    {
        int colonIndex = dispenseTime.indexOf(':');
        if (colonIndex == -1 || colonIndex == 0 || colonIndex >= dispenseTime.length() - 1)
        {
            return -1; // Invalid format
        }

        String minuteStr = dispenseTime.substring(colonIndex + 1);
        int minute = minuteStr.toInt();

        if (minute < 0 || minute > 59)
        {
            return -1; // Invalid minute
        }

        return minute;
    }

    int getTotalMinutesSinceMidnight() const
    {
        int hours = getHours();
        int minutes = getMinutes();

        if (hours == -1 || minutes == -1)
        {
            Serial.println("getTotalMinutesSinceMidnight: Invalid hour or minutes");
            return -1;
        }

        return 60 * hours + minutes;
    }
};

#define MAX_ENTRIES_NUM 1440 // Maximum number of entries (one per minute in a day)

struct FeedConfigData
{
    static FeedConfigEntry configEntries[MAX_ENTRIES_NUM]; // Array to store feed config entries
    static int numOfEntries; // Number of valid entries in the array

    FeedConfigData(const String& feedFoodConfigurationJSON)
    {
        numOfEntries = 0;

        // Parse the JSON
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, feedFoodConfigurationJSON);

        if (error)
        {
            Serial.print("Failed to parse JSON: ");
            Serial.println(error.c_str());
            return;
        }

        // Iterate over the JSON object
        for (JsonPair kv : doc.as<JsonObject>())
        {
            if (numOfEntries >= MAX_ENTRIES_NUM)
            {
                Serial.println("Exceeded maximum number of entries!");
                break;
            }

            // Extract time (key) and amount (value)
            String dispenseTime = kv.key().c_str();
            int quantity = kv.value().as<int>();

            // Populate the FeedConfigEntry
            configEntries[numOfEntries].dispenseTime = dispenseTime;
            configEntries[numOfEntries].quantity = quantity;
            configEntries[numOfEntries].wasDispensedToday = false;

            numOfEntries++; // Increment the number of entries
        }

        // Database entries may not be sorted, so we sort them to ensure a chronological order
        sortEntriesByTime();

        Serial.println("FeedConfigData deserialization completed.");
    }

    String toString() const
    {
        String result = "FeedConfigData:\n";

        for (int i = 0; i < numOfEntries; i++)
        {
            result += "  Entry " + String(i + 1) + ": " + configEntries[i].toString() + "\n";
        }

        return result;
    }

private:
    void sortEntriesByTime()
    {
        // Sort entries by time using bubble sort
        for (int i = 0; i < numOfEntries - 1; i++)
        {
            for (int j = i + 1; j < numOfEntries; j++)
            {
                if (configEntries[i].getTotalMinutesSinceMidnight() > configEntries[j].getTotalMinutesSinceMidnight())
                {
                    // Swap entries
                    FeedConfigEntry temp = configEntries[i];
                    configEntries[i] = configEntries[j];
                    configEntries[j] = temp;
                }
            }
        }
    }
};

// Initialize static members
FeedConfigEntry FeedConfigData::configEntries[MAX_ENTRIES_NUM];
int FeedConfigData::numOfEntries = 0;

enum TrapMode
{
    TAG_BASED,
    ALWAYS_OPEN
};

struct FeederConfigData
{
    String wifiSSID;
    String wifiPassword;
    String feedFoodConfigurationJSON;
    TrapMode trapMode;
};

#endif // FEEDER_CONFIG_H