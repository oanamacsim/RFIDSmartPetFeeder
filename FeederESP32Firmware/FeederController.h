#include <MemoryController.h>
#include <WeightController.h>
#include <WebConnectionController.h>
#include <FeederDataTypes.h>
#include <GateController.h>

static int getCurrentDayFromUnix(unsigned long unixTime)
{
    return ((unixTime / 86400L) + 4) % 7; // 1970-01-01 was a Thursday (4), adjusting to 0 = Sunday
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

class FeederController
{
private:
    struct RelayPins 
    {
        static constexpr int MOTOR_CONTROL = 21;
    };

    MemoryController* memoryController = nullptr;
    WeightController* weightController = nullptr;
    WebConnectionController* webConnection = nullptr;
    GateController* gateController = nullptr;

    FeedConfigData* feedConfigData = nullptr;

    int currentDay; // 0-6, 0-monday, ... 6-sunday
    int minutesSinceMidnight; // 0-1440
    bool canFeedByTime = false;
public:
    bool isFeeding = false;

    FeederController(MemoryController* memController, WeightController* weightCtrl, WebConnectionController* webConn, GateController* gateCtrl)
    {
        memoryController = memController;
        weightController = weightCtrl;
        webConnection = webConn;
        gateController = gateCtrl;

        feedConfigData = new FeedConfigData(memoryController->getFoodConfigJson());

        canFeedByTime = false; 

        initializeFeederTimeParams();
    }

    void resetFeedConfigDataDispenseStatus()
    {
        Serial.println("resetFeedConfigDataDispenseStatus");

        for(int i=0;i<feedConfigData->numOfEntries;i++)
        {
            int configEntryMinutesSinceMidnight = feedConfigData->configEntries[i].getTotalMinutesSinceMidnight();

            if(configEntryMinutesSinceMidnight >= minutesSinceMidnight)
            {
                // Consider all config dates after the current minutesSinceMidnight as not dispensed
                feedConfigData->configEntries[i].wasDispensedToday = false;
            }
            else
            {
                // Consider all config dates before the current minutesSinceMidnight as dispensed already
                feedConfigData->configEntries[i].wasDispensedToday = true;
            }
        }
    }

    void initializeFeederTimeParams()
    {
        unsigned long currentTime = webConnection->getCurrentTime(true);
        currentDay = getCurrentDayFromUnix(currentTime);
        minutesSinceMidnight = getRelativeMinutesSinceMidnight(currentTime);

        if(currentTime == 0)
        {
            canFeedByTime = false; // time was not synched yet, so we are unable to dispense the food by the timing logic
            Serial.println("FeederController canFeedByTime=false because the time was not synched. It will be true when the time will sync correctly");
        }
        else
        {
            canFeedByTime = true;
            resetFeedConfigDataDispenseStatus();
        }
    }

    unsigned long lastTriggerTime = 0;
    const unsigned long interval = 5000;
    void loop()
    {
        // Because the loop function is CPU heavy, we're gonna trigger it once 5 seconds
        if (millis() - lastTriggerTime < interval) 
        {
            return;
        }

        if(!canFeedByTime)
        {
            return;
        }

        lastTriggerTime = millis();        

        if(getCurrentDayFromUnix(webConnection->getCurrentTime(true)) != currentDay)
        {
            Serial.println("One day just passed. Reseting feed config program");

            // The day just changed, so we have to reset the feederConfig array to prepare for a new day
            currentDay = getCurrentDayFromUnix(webConnection->getCurrentTime(true));
            minutesSinceMidnight = 0; // 0 minutes from the midnight

            resetFeedConfigDataDispenseStatus();
        }

        int currentMinutesSinceMidnight = getRelativeMinutesSinceMidnight(webConnection->getCurrentTime(true));

        for(int i=0;i<feedConfigData->numOfEntries;i++)
        {
            if(feedConfigData->configEntries[i].wasDispensedToday == true)
            { 
                continue; // Skip already dispensed configs
            }

            int configEntryMinutesSinceMidnight = feedConfigData->configEntries[i].getTotalMinutesSinceMidnight();

            if(currentMinutesSinceMidnight >= configEntryMinutesSinceMidnight)
            {
                dispenseFeedConfigQuantity(feedConfigData->configEntries[i]);
            }
        }
    }
    
    void dispenseFeedConfigQuantity(FeedConfigEntry& feedConfigEntry)
    {
        Serial.println("DispenseFeedConfigQuantity for entry: " + feedConfigEntry.dispenseTime + " with quantity" + String(feedConfigEntry.quantity) + "gr");

        Serial.println("Closing the gate before starting the dispense");

        int initialWeight = weightController->getWeight();
        int currentWeight = initialWeight;
        int expectedWeight = initialWeight + (int)feedConfigEntry.quantity;
        
        expectedWeight = min(60, expectedWeight);

        int currentTime = millis();
        int maxExceededTime = 70000;
        startFeeding();

        String feedStatus;

        int previousWeight = -1;
        while(currentWeight < expectedWeight)
        {
            if(millis() - currentTime > maxExceededTime)
            {
                // Unable to dispanse the wanted amount in the 45 seconds. Check wirings or foodStorage
                break;
            }

            previousWeight = currentWeight;
            currentWeight = weightController->getWeight();
            Serial.println("DispenseFeedConfigQuantity. CurrentWeight: " + String(currentWeight) + " , expected: " + String(expectedWeight));
            
            delay(1000);
        }
        
        stopFeeding();

        if(currentWeight >= expectedWeight)
        {
            Serial.println("Food dispensed complete. Amount dispensed: " + String(feedConfigEntry.quantity));
            webConnection->addFoodDispenseEvent(webConnection->getCurrentTime(), feedConfigEntry.quantity);
        }
        else if(currentWeight > initialWeight + 5) // 5 grams added to avoid small errors
        {
            Serial.println("Food dispensed partially. Amount dispensed: " + String(currentWeight - initialWeight));
            webConnection->addFoodDispenseEvent(webConnection->getCurrentTime(), currentWeight - initialWeight);
        }
        else
        {
            Serial.println("Unable to dispanse the wanted amount in the 45 seconds. Check wirings or foodStorage");
            webConnection->addFoodDispenseEvent(webConnection->getCurrentTime(), 0);
        }

        feedConfigEntry.wasDispensedToday = true;
        webConnection->updateFoodWeight(currentWeight, webConnection->getCurrentTime());
    }

    void startFeeding()
    {
        Serial.println("Start feeding called. isFeeding: " + String((int)isFeeding));
        if(!isFeeding)
        {
          Serial.println("Start feeding");
          // Activate the relay to start feeding
          digitalWrite(RelayPins::MOTOR_CONTROL, LOW);
          isFeeding = true;
        }
    }

    void stopFeeding()
    {
        Serial.println("Stop feeding called. isFeeding: " + String((int)isFeeding));
        if(isFeeding)
        {
            Serial.println("Stop feeding");
            // Deactivate the relay to stop feeding
            digitalWrite(RelayPins::MOTOR_CONTROL, HIGH);
            isFeeding = false;
        }
    }
};