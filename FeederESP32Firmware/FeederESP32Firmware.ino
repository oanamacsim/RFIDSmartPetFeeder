#include "FeederController.h"
#include "GateController.h"
#include "RFIDController.h"
#include "WeightController.h"
#include "WifiController.h"
#include "MemoryController.h"

// Global instances of controllers
FeederController* feederController = nullptr;
GateController* gateController = nullptr;
RFIDController* rfidController = nullptr;
WeightController* weightController = nullptr;
WifiController* wifiController = nullptr;
MemoryController* memoryController = nullptr;

// Forward declarations
void initializeControllers();
void initializeRelayPin();
void processCommandsFromApp();
void updateFoodWeightRecurrently();
void handleCommand(const String& command);

void setup() 
{
    Serial.begin(115200);
    delay(1000);

    initializeControllers();

    int weightToLoadAtRestart = memoryController->getWeightToLoadAtRestart();
    if (weightToLoadAtRestart > 0)
    {   
        Serial.println("CatFeeder.ino set weightToLoadAtRestart: " + String(weightToLoadAtRestart));
        weightController->setInitialOffset(weightToLoadAtRestart);
    }

    feederController = new FeederController(memoryController, weightController, wifiController->getWebConnection(), gateController);
}

void loop() 
{   
    synchTime();

    rfidController->loop();
    gateController->loop();
    wifiController->loop();
    feederController->loop();

    processCommandsFromApp();
    updateFoodWeightRecurrently();

    delay(100);
}

void initializeControllers()
{
    memoryController = new MemoryController();
    wifiController = new WifiController(memoryController);
    gateController = new GateController(wifiController->getWebConnection());
    rfidController = new RFIDController(gateController);
    weightController = new WeightController();
}

void synchTime()
{
    if (wifiController->getWebConnection()->haveInternetConnection())
    {
        if (!wifiController->getWebConnection()->getCurrentTime())
        {
            wifiController->getWebConnection()->synchronizeTime();
            if (wifiController->getWebConnection()->getCurrentTime())
            {
                feederController->initializeFeederTimeParams();
            }
        }
    }
}

void processCommandsFromApp()
{
    static const int COMMAND_PROCESS_INTERVAL = 5000; // 5 seconds
    static unsigned long lastProcessedCommand = 0;

    if (millis() - lastProcessedCommand < COMMAND_PROCESS_INTERVAL)
    {
        return; // Skip processing if interval not reached
    }

    String command = wifiController->getWebConnection()->getCommandFromApplication();
    if (command.length() > 0)
    {
        Serial.println("Command from app received: " + command);
        handleCommand(command);
    }

    lastProcessedCommand = millis();
}

void handleCommand(const String& command)
{
    if (command.indexOf("UpdateFeeder") != -1)
    {   
        Serial.println("Command UpdateFeeder, setWeightToLoadAtRestart: " + String(weightController->getWeight()));
        memoryController->setWeightToLoadAtRestart(weightController->getWeight());
        ESP.restart();
    }
    else if (command.indexOf("DispenseNow") != -1)
    {
        int underscoreIndex = command.indexOf('_');
        String quantityStr = command.substring(underscoreIndex + 1);
        float quantity = quantityStr.toFloat();

        FeedConfigEntry feedConfig;
        feedConfig.dispenseTime = "00:00";
        feedConfig.quantity = quantity;
        feedConfig.wasDispensedToday = false;

        feederController->dispenseFeedConfigQuantity(feedConfig);
    }
}

void updateFoodWeightRecurrently()
{
    static const int FOOD_WEIGHT_UPDATE_INTERVAL = 300000; // 5 minutes
    static unsigned long lastProcessedFoodWeightUpdate = 0;

    if (millis() - lastProcessedFoodWeightUpdate < FOOD_WEIGHT_UPDATE_INTERVAL)
    {
        return; // Skip processing if interval not reached
    }

    wifiController->getWebConnection()->updateFoodWeight(weightController->getWeight(), wifiController->getWebConnection()->getCurrentTime());
    lastProcessedFoodWeightUpdate = millis();
}