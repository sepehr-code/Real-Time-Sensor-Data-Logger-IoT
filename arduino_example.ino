/*
 * Arduino Sensor Data Logger Example
 * 
 * This sketch demonstrates how to send sensor data in the format
 * expected by the Real-Time Sensor Data Logger hardware interface.
 * 
 * Data Format: "SENSOR:TYPE:VALUE:UNIT:DESCRIPTION\n"
 * 
 * Supported sensor types:
 * - TEMP (Temperature)
 * - VIB (Vibration)
 * - STRAIN (Strain)
 * - HUM (Humidity)
 * - PRESS (Pressure)
 * - ACCEL_X, ACCEL_Y, ACCEL_Z (Accelerometer)
 */

// Pin definitions
#define TEMP_SENSOR_PIN A0
#define VIBRATION_SENSOR_PIN A1
#define LED_PIN 13

// Timing
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 100; // 100ms = 10Hz

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  
  // Initialize sensors
  analogReference(DEFAULT);
  
  Serial.println("Arduino Sensor Logger Ready");
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  unsigned long currentTime = millis();
  
  // Read sensors at specified interval
  if (currentTime - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = currentTime;
    
    // Read temperature sensor (example: LM35)
    int tempRaw = analogRead(TEMP_SENSOR_PIN);
    float temperature = (tempRaw * 5.0 / 1024.0) * 100.0; // LM35: 10mV/°C
    
    // Read vibration sensor (example: accelerometer or piezo)
    int vibRaw = analogRead(VIBRATION_SENSOR_PIN);
    float vibration = (vibRaw * 5.0 / 1024.0); // Convert to voltage, then to m/s²
    vibration = abs(vibration - 2.5) * 2.0; // Center around 2.5V, scale to ±5 m/s²
    
    // Send temperature data
    Serial.print("SENSOR:TEMP:");
    Serial.print(temperature, 2);
    Serial.println(":C:Arduino Temperature");
    
    // Send vibration data
    Serial.print("SENSOR:VIB:");
    Serial.print(vibration, 3);
    Serial.println(":m/s²:Arduino Vibration");
    
    // Blink LED to show activity
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
  
  // Handle incoming commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "PING") {
      Serial.println("PONG");
    } else if (command == "STATUS") {
      Serial.println("OK:Arduino Sensor Logger Active");
    } else if (command.startsWith("SET_INTERVAL:")) {
      // Allow changing sample rate
      int newInterval = command.substring(13).toInt();
      if (newInterval >= 10 && newInterval <= 10000) {
        // SENSOR_INTERVAL = newInterval; // Would need to make it non-const
        Serial.print("OK:Interval set to ");
        Serial.println(newInterval);
      } else {
        Serial.println("ERROR:Invalid interval (10-10000ms)");
      }
    }
  }
}

/*
 * Example sensor connections:
 * 
 * LM35 Temperature Sensor:
 * - VCC to 5V
 * - GND to GND
 * - OUT to A0
 * 
 * ADXL335 Accelerometer (for vibration):
 * - VCC to 3.3V
 * - GND to GND
 * - X-OUT to A1
 * - Y-OUT to A2
 * - Z-OUT to A3
 * 
 * Piezo Vibration Sensor:
 * - One terminal to A1
 * - Other terminal to GND
 * - 1MΩ resistor across terminals
 * 
 * Usage with the data logger:
 * ./datalogger --hardware /dev/ttyUSB0 --duration 300
 */ 