#ifndef GATE_CONTROLLER_H
#define GATE_CONTROLLER_H

#include "WebConnectionController.h"
#include <Stepper.h>

class GateController
{
private:
    
    enum class GateOperation 
    {
        NONE,
        OPEN,
        CLOSE
    };

    struct StepperPins 
    {
        // Pin definitions for the stepper motor
        static constexpr int IN1 = 19;
        static constexpr int IN2 = 18;
        static constexpr int IN3 = 5;
        static constexpr int IN4 = 17;
    };

    static constexpr int STEPS_PER_REVOLUTION = 2048;

    // Stepper motor initialization
    Stepper stepperMotor{STEPS_PER_REVOLUTION, StepperPins::IN1, StepperPins::IN3, StepperPins::IN2, StepperPins::IN4};

    unsigned long actionEndTime = 0; // Timestamp to track when the current action ends
    static constexpr unsigned long WAIT_TIME_AFTER_ACTION = 3000; // Wait time after an action (in milliseconds)

    GateOperation lastOperation = GateOperation::NONE; // Tracks the last operation performed

    bool gateWasOpened = false; // Tracks if the gate was opened
    unsigned long openTimestamp = 0; // Timestamp when the gate was opened
    unsigned long closeTimestamp = 0; // Timestamp when the gate was closed

    WebConnectionController* webConnection = nullptr;

    void deactivateStepperPins()
    {
        digitalWrite(StepperPins::IN1, LOW);
        digitalWrite(StepperPins::IN2, LOW);
        digitalWrite(StepperPins::IN3, LOW);
        digitalWrite(StepperPins::IN4, LOW);
    }

public:

    GateController(WebConnectionController* webConnectionController) : webConnection(webConnectionController)
    {
        Serial.println("GateController Constructor");

        // Initialize stepper motor pins
        pinMode(StepperPins::IN1, OUTPUT);
        pinMode(StepperPins::IN2, OUTPUT);
        pinMode(StepperPins::IN3, OUTPUT);
        pinMode(StepperPins::IN4, OUTPUT);

        deactivateStepperPins(); // Ensure the stepper motor is deactivated initially
        stepperMotor.setSpeed(5); // Set the stepper motor speed
    }

    void loop()
    {
        if (!isBusy())
        {
            deactivateStepperPins();
        }
    }

    // Check if the gate controller is busy (motor moving or within wait time)
    bool isBusy()
    {
        return millis() < actionEndTime;
    }

    // Open the gate
    void open()
    {
        if (isBusy() || lastOperation == GateOperation::OPEN)
            return;

        Serial.println("Opening gate...");

        stepperMotor.step(-625); // Move the stepper motor to open the gate
        actionEndTime = millis() + WAIT_TIME_AFTER_ACTION; // Update the action end time

        if (webConnection)
        {
            openTimestamp = webConnection->getCurrentTime(); // Record the open timestamp
        }

        // Update state variables
        gateWasOpened = true;
        lastOperation = GateOperation::OPEN;
    }

    // Close the gate
    void close(bool updateDatabase = true)
    {
        if (isBusy() || lastOperation == GateOperation::CLOSE)
            return;

        Serial.println("Closing gate...");

        stepperMotor.step(460); // Move the stepper motor to close the gate
        actionEndTime = millis() + WAIT_TIME_AFTER_ACTION; // Update the action end time

        if (webConnection && updateDatabase)
        {
            closeTimestamp = webConnection->getCurrentTime(); // Record the close timestamp
            if (openTimestamp != 0 && closeTimestamp != 0)
            {
                webConnection->addGateEvent(openTimestamp, closeTimestamp); // Update the database
            }
        }

        // Update state variables
        openTimestamp = 0;
        closeTimestamp = 0;
        gateWasOpened = false;
        lastOperation = GateOperation::CLOSE;
    }
};

#endif // GATE_CONTROLLER_H