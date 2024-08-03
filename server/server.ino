#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char *ssid = "DASS-IOT";
const char *password = "Dass0306IOT";
const char *serverUrl = "http://10.110.90.192:2399/diesel"; 
const int sensorPin = 4;
int sensorState = 0;

void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    Serial.print("Connecting to: ");
    Serial.println(ssid);
  }

  Serial.println("Connected to WiFi");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    int level_test = 0;

    sensorState = digitalRead(sensorPin);

    if (sensorState != HIGH) {
        level_test += 1;
        Serial.print("Nivel de Diesel: ");
        Serial.println(level_test);
        enviarDados(level_test);
      }   

    delay(1000);
  }
}

void enviarDados(int level) {
 
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(15000);  
    Serial.println(client.getLastSSLError());

    HTTPClient http;
    http.setTimeout(15000);
    
    String serverPath = String(serverUrl);
    String jsonData = "{\"level\":" + String(level) + "}";

    Serial.println("JSON a ser enviado: " + jsonData);


    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(jsonData);;

    if (httpCode > 0) {
      Serial.printf("[HTTP] POST realizado com sucesso, código: %d\n", httpCode);
      String resposta = http.getString();  

      Serial.println("Resposta do servidor: " + resposta);

    } else {
      Serial.print("[HTTP] Falha na requisição POST, código de erro: ");
      Serial.println(http.errorToString(httpCode).c_str());
    }
    http.end();
}