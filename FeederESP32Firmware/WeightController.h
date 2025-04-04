#ifndef WEIGHT_CONTROLLER_H
#define WEIGHT_CONTROLLER_H

#include "HX711.h"
#include "MemoryController.h"

class WeightController
{
private:

    struct ScalePins
    {
        // Pin definitions for the HX711
        static constexpr int DATA_PIN = 27;  // Data pin for the HX711
        static constexpr int CLOCK_PIN = 14; // Clock pin for the HX711
    };

    HX711 scale; // HX711 load cell amplifier instance

    // Calibration factor for the scale
    float calibrationFactor = 466170.09;

    // Cached weight value to return if the scale is not ready
    int cachedWeight = -1;

    // Offset to adjust the weight readings
    int offset = 0;

public:
    static constexpr float INVALID_WEIGHT_VALUE = -1.0f;

    WeightController()
    {
        scale.begin(ScalePins::DATA_PIN, ScalePins::CLOCK_PIN);
        scale.set_scale(calibrationFactor); // Set calibration factor
        scale.tare(); // Reset the scale to zero
    }

    void setCalibrationFactor(float calibFact)
    {
        Serial.println("WeightController: Calibration factor set");
        calibrationFactor = calibFact;
        scale.set_scale(calibrationFactor);
    }

    float getCalibrationFactor() const
    {
        return calibrationFactor;
    }

    // When the feeder restarts, use a saved offset for food dispensed before the restart
    void setInitialOffset(int offsetToSet)
    {
        offset = offsetToSet;
    }

    // Get the current weight in grams
    int getWeight()
    {
        if (scale.is_ready())
        {
            // Read the weight, apply the offset, and ensure it's non-negative
            float rawWeight = scale.get_units() * 1000.0f; // Convert to grams
            cachedWeight = max(0, static_cast<int>(rawWeight)) + offset;
            return cachedWeight;
        }
        else
        {
            Serial.println("WeightController: HX711 not found. Returning cached weight");
            return cachedWeight; // Return the last valid weight if the scale is not ready
        }
    }
};

#endif // WEIGHT_CONTROLLER_H