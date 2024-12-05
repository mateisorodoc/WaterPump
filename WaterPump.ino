// Define pins
const int pumpRelayPin = 2;         // Relay connected to pump (active low)
const int soilMoisturePin = A0;     // Capacitive soil moisture sensor
const int waterLevelPin = A1;       // Water level sensor (analog)
const int flowMeterPin = 3;         // Flowmeter signal pin

// Set thresholds
const int soilMoistureThreshold = 350;  // Adjust this value based on calibration
const int waterLevelThreshold = 720;    // Adjust this value based on calibration
const float waterVolumeTarget = 50.0;   // Target water volume in ml (for one watering cycle)

// Flowmeter variables
volatile int flowPulseCount = 0;
const float calibrationFactor = 5000; // Calibration factor (pulses per liter)

// Variables
float totalVolume = 0.0; // Total volume delivered (ml)
unsigned long lastWateringTime = 0; // Last time the soil moisture was checked
const unsigned long wateringInterval = 10000; // 10 seconds in milliseconds (change as needed)
unsigned long sensorReadInterval = 5000; // 5 seconds interval for sensor readings
unsigned long lastSensorReadTime = 0; // Last time the sensors were read

// Variables for hysteresis
const int soilMoistureHysteresis = 50;  // Hysteresis value to avoid rapid switching

void flowPulseCounter() {
    flowPulseCount++;
}

void setup() {
    // Initialize serial communication
    Serial.begin(9600);

    // Configure pins
    pinMode(pumpRelayPin, OUTPUT);
    pinMode(flowMeterPin, INPUT_PULLUP);

    // Attach interrupt for the flowmeter
    attachInterrupt(digitalPinToInterrupt(flowMeterPin), flowPulseCounter, RISING);

    // Ensure pump is off initially (relay inactive)
    digitalWrite(pumpRelayPin, HIGH);

    // Introductory message
    Serial.println("System Initialized. Monitoring Sensors...");
}

void loop() {
    unsigned long currentMillis = millis();

    // Read sensor values every 5 seconds
    if (currentMillis - lastSensorReadTime >= sensorReadInterval) {
        // Read soil moisture sensor
        int soilMoistureValue = analogRead(soilMoisturePin);

        // Read water level sensor
        int waterLevelValue = analogRead(waterLevelPin);

        // Check if water is available based on the threshold
        bool waterAvailable = (waterLevelValue > waterLevelThreshold);

        // Display sensor readings
        Serial.println("=== Sensor Readings ===");
        Serial.print("Soil Moisture: ");
        Serial.println(soilMoistureValue);
        Serial.print("Water Level: ");
        Serial.println(waterLevelValue);
        Serial.print("Water Available: ");
        Serial.println(waterAvailable ? "Yes" : "No");
        Serial.print("Total Volume Delivered: ");
        Serial.print(totalVolume);
        Serial.println(" ml");
        Serial.println("========================");

        // Update last sensor read time
        lastSensorReadTime = currentMillis;
    }

    // Check if 30 minutes have passed since the last watering
    if (currentMillis - lastWateringTime >= wateringInterval) {
        // Read soil moisture sensor again after 30 minutes
        int soilMoistureValue = analogRead(soilMoisturePin);

        // Read water level sensor
        int waterLevelValue = analogRead(waterLevelPin);

        // Check if water is available based on the threshold
        bool waterAvailable = (waterLevelValue > waterLevelThreshold);

        // Apply hysteresis to avoid rapid switching
        if (soilMoistureValue > (soilMoistureThreshold + soilMoistureHysteresis)) {
            soilMoistureValue = soilMoistureThreshold + soilMoistureHysteresis;
        } else if (soilMoistureValue < (soilMoistureThreshold - soilMoistureHysteresis)) {
            soilMoistureValue = soilMoistureThreshold - soilMoistureHysteresis;
        }

        // If soil moisture is above the threshold (dry) and water is available, start pump
        if (soilMoistureValue > soilMoistureThreshold && waterAvailable) {
            flowPulseCount = 0; // Reset flowmeter pulse count
            totalVolume = 0.0; // Reset total volume for this cycle
            digitalWrite(pumpRelayPin, LOW); // Turn on pump (active low)
            Serial.println("Pump ON");

            while (totalVolume < waterVolumeTarget) {
                // Calculate volume based on flowmeter pulses
                float flowRate = (flowPulseCount / calibrationFactor) * 1000; // Flow rate in ml/min
                unsigned long pumpingDuration = millis();

                // Update the total volume delivered
                totalVolume = (flowPulseCount / calibrationFactor) * 1000; // Convert to ml

                // Display live flow update
                Serial.println("--- Flow Update ---");
                Serial.print("Flow Rate: ");
                Serial.print(flowRate);
                Serial.println(" ml/min");
                Serial.print("Total Volume: ");
                Serial.print(totalVolume);
                Serial.println(" ml");
                Serial.println("-------------------");

                // Ensure the pump stops once the target volume is reached
                if (totalVolume >= waterVolumeTarget) break;
            }

            // Stop the pump after the target volume is dispensed
            digitalWrite(pumpRelayPin, HIGH); // Turn off pump (active low)
            Serial.println("Pump OFF");

            // Record the time of this watering cycle
            lastWateringTime = millis();
        }

        // If soil is sufficiently wet, turn off pump
        if (soilMoistureValue <= soilMoistureThreshold) {
            digitalWrite(pumpRelayPin, HIGH); // Turn off pump
            Serial.println("Soil moisture sufficient. Pump OFF.");
        }
    }
}
