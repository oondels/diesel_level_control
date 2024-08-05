#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Ultrasonic.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SERVER_IP "http://10.110.20.101:2399/post-diesel"

// Wifi Configuration
#ifndef SSID
#define SSID "DASS-CORP"
#define PASS "dass7425corp"
#endif

// Sensor Configuration
#define TRIGGER_PIN  13
#define ECHO_PIN     5 

Ultrasonic ultrasonic(TRIGGER_PIN, ECHO_PIN);

// Reading Interval (5 seconds)
unsigned long previousMillis = 0;
const long interval = 3000;

float measureVolume(float distance_from_sensor) {
  // Dados do copo
  // V = 1/3 πh(R² + Rr + r²)

  float h_total = 8.46; 
  float R = 3.40;     
  float r = 2.30;    

  // Calcula a altura preenchida
  float h_filled = h_total - distance_from_sensor;

  // Calcula o volume atual preenchido
  float v_total = (1.0 / 3.0) * h_total * M_PI * (pow(R, 2) + R * r + pow(r, 2));
  float v_filled = (1.0 / 3.0) * h_filled * M_PI * (pow(R, 2) + R * r + pow(r, 2));

  return v_filled; 
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Connect to Wi-Fi
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  delay(50);
  
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      HTTPClient http;

      // Sensor Measuring
      digitalWrite(TRIGGER_PIN, LOW);
      delayMicroseconds(2);
      digitalWrite(TRIGGER_PIN, HIGH);
      delayMicroseconds(10);
      digitalWrite(TRIGGER_PIN, LOW);

      long duration = pulseIn(ECHO_PIN, HIGH);
      float distance = duration * 0.0343 / 2;
      float volume = measureVolume(distance);

      if (distance == 0) {
        Serial.println("Erro no sensor");
      } else {
        // Specify the URL
        http.begin(client, SERVER_IP);
        http.addHeader("Content-Type", "application/json");

        Serial.print("Sending data...\n");
        
        // Data to send with HTTP POST
        char dieselData[100];
        snprintf(dieselData, sizeof(dieselData), "{\"nivel\":%.4f,\"unidade_dass\":1,\"distance\":%.4f}", volume);

        
        
        // Send HTTP POST request
        int httpResponseCode = http.POST(dieselData);

    

        // Check the returning code
        if (httpResponseCode > 0) {
          // HTTP header has been sent and Server response header has been handled
          Serial.printf("POST code: %d\n", httpResponseCode);

          if (httpResponseCode == HTTP_CODE_OK) {
            const String& result = http.getString();
            Serial.println("<<");
            Serial.print("Response: ");
            Serial.println(result); 
            Serial.println(">>");
          }

        } else {
          Serial.print("Error on sending POST: ");
          Serial.println(http.errorToString(httpResponseCode).c_str());
        }

        http.end();
      }
    } else {
      Serial.println("WiFi Disconnected");
    }
  }
}
