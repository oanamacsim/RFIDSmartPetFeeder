#ifndef RFID_CONTROLLER_H
#define RFID_CONTROLLER_H

#include <Arduino.h>
#include <rdm6300.h>
#include "GateController.h"

class RFIDController
{
private:

    struct RFIDPins
    {
        // Pin definitions for the RFID reader
        static constexpr byte RX_PIN = 4; // RX pin for receiving data from the RDM6300
    };

    // Registered RFID tags (For each feeder we have the posibility to register two tags)
    String registeredTag1;
    String registeredTag2;

    GateController* gateController = nullptr;

    // Timestamp to track the last time a registered tag was read
    unsigned long lastTagReadTime = 0;

    // Flag to track if a registered tag was read
    bool registeredTagWasRead = false;

    Rdm6300 rdm6300;

public:

    RFIDController(GateController* gate_controller) : gateController(gate_controller)
    {
        Serial.println("RFIDController Constructor...");

        // Initialize the RDM6300 reader
        rdm6300.begin(RFIDPins::RX_PIN);

        // Set default registered tags
        setRegisteredTag1("7E3FE9");
        setRegisteredTag2("1ECADE");
    }

    // Read an RFID tag (if any exists within its detection range)
    String readTag()
    {
        if (rdm6300.get_tag_id())
        {
            String tagHex = String(rdm6300.get_tag_id(), HEX);
            tagHex.toUpperCase();
            return tagHex;
        }
        return "";
    }

    void loop()
    {
        const String& tagHex = readTag();

        if (!tagHex.isEmpty())
        {
            if (tagHex.equals(registeredTag1) || tagHex.equals(registeredTag2))
            {
                lastTagReadTime = millis(); // Update the last read time for the registered tag
                registeredTagWasRead = true;
            }
            else
            {
                lastTagReadTime = 0; // Invalidate the timer for unregistered tags
                registeredTagWasRead = false;
            }
        }

        // Control the gate based on whether a registered tag is present
        if (isRegisteredTagPresent())
        {
            gateController->open();
        }
        else
        {
            gateController->close();
        }
    }

    void setRegisteredTag1(const String& tag)
    {
        registeredTag1 = tag;
    }

    void setRegisteredTag2(const String& tag)
    {
        registeredTag2 = tag;
    }

    // Check if a registered tag was read within the last `tagTimeout` milliseconds
    bool isRegisteredTagPresent(int tagTimeout = 10000) // Default timeout: 10 seconds
    {
        return registeredTagWasRead && (millis() - lastTagReadTime) <= tagTimeout;
    }

    // Invalidate the registered tag (reset the timer and flag)
    void invalidateRegisteredTag()
    {
        registeredTagWasRead = false;
        lastTagReadTime = 0;
    }
};

#endif // RFID_CONTROLLER_H