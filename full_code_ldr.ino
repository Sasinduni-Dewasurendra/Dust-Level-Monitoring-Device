#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Replace with your WiFi credentials
const char* ssid = "Huawei y9 prime";
const char* password = "12345678";

// Replace with your MQTT broker details
const char* mqtt_server = "test.mosquitto.org";

const char* mqtt_topic = "dust sensor";
const char* mqtt_topic_output = "ldr_sensor_output"; // Topic for output
const char* mqtt_topic_1 = "ldr_sensor1"; //   Topic for output
const char* mqtt_topic_2 = "ldr_sensor2"; // Topic for output
const char* mqtt_topic_3 = "ldr_sensor3"; // Topic for output
const char* mqtt_topic_4 = "ldr_sensor4"; // Topic for output
const char* mqtt_topic_text = "text";

const int LDR_PINS[3] = {33, 34, 35};
const int LDR_1 = analogRead(LDR_PINS[0]); // Assuming initial readings available
const int LDR_2 = analogRead(LDR_PINS[1]);
const int LDR_3 = analogRead(LDR_PINS[2]);

int pin = 25; 
unsigned long duration;

const float referenceVoltage = 5.0f; // Assuming 5V power supply for Arduino Uno

int outputHistory[10]; // Array to store last 10 output values (adjust size)
int cleanCount = 0;

String cleanStatusMessage;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(9600); // Start serial communication for printing data (optional)
  analogReadResolution(12);

  pinMode(pin, INPUT);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // Connect to MQTT broker
  client.setServer(mqtt_server, 1883);
}

void loop() {
  // Check MQTT connection and reconnect if needed
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Read pulse width
  duration = pulseInLong(pin, LOW);

  int sensorReadings[3];  // Array to store sensor readings

  for (int i = 0; i < 3; i++) {
    sensorReadings[i] = analogRead(LDR_PINS[i]);
  }

  Serial.println("Sensor Readings:");
  for (int i = 0; i < 3; i++) {
    Serial.print("LDR ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(sensorReadings[i]);
  }
  
  int output;

  // Check for all LDR values between 3500 and 4095
  bool allWithinRange = true;
  for (int i = 0; i < 3; i++) {
    if (sensorReadings[i] < 4000 || sensorReadings[i] > 4095) {
      allWithinRange = false;
      break;
    }
  }

  // Check for all within 3000 and 4095 (original logic)
  int countWithinRanges = 0;
  for (int i = 0; i < 3; i++) {
    if (sensorReadings[i] >= 3000 && sensorReadings[i] <= 4095) {
      countWithinRanges++;
    }
  }

  if (allWithinRange) {
    output = 0;  // All LDR values between 3500 and 4095
  } else if (countWithinRanges == 3) {  // All within 3000 and 4095 (unchanged)
    output = 1;
  } else {
    output = 2;  // None between 3500-4095 or not all within 3000-4095
  }

  // Update output history
  outputHistory[(cleanCount % 10)] = output; // Circular buffer approach

  // Check for consistent output (2)
  cleanCount = 0;
  for (int i = 0; i < 10; i++) {
    if (outputHistory[i] == output) {
      cleanCount++;
    } else {
      break;
    }
  }

  // Print message based on clean count and output value
  if (cleanCount == 10) {
    if (output == 0) {
      cleanStatusMessage = "Solar panel is clean";
    } else if (output == 1) {
      cleanStatusMessage = "Solar panel is not clean";
    } else if(output == 2){
      cleanStatusMessage = "Urgently clean the solar panel";
    }
    cleanCount = 0; // Reset clean count (optional)
  } 

  // Publish output to MQTT topic
  client.publish(mqtt_topic, String(duration).c_str());
  client.publish(mqtt_topic_output, String(output).c_str());
  client.publish(mqtt_topic_1, String(sensorReadings[0]).c_str());
  client.publish(mqtt_topic_2, String(sensorReadings[1]).c_str());
  client.publish(mqtt_topic_3, String(sensorReadings[2]).c_str());
  client.publish(mqtt_topic_text, cleanStatusMessage.c_str());

  // Print sensor values and output (optional)
  Serial.print("Dust sensor value: ");
  Serial.println(duration);

  Serial.print("Output: ");
  Serial.println(output); // Print the output value (0, 1, or 2)

  delay(1000); // Delay between readings (adjust as needed)
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("ESP32_Dust_Sensor")) {  // Give a unique client ID
      Serial.println("Connected to MQTT broker");
    } else {
      Serial.print("Failed to connect, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
