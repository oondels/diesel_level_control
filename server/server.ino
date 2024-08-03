#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <SimplePgSQL.h>

const char *ssid = "DASS-CORP";
const char *pass = "dass7425corp";
WiFiClient client;

// Setup connection and Database
IPAddress PGIP(192, 168, 26, 90);
const char user[] = "postgres";
const char password[] = "gdti5s11se";
const char dbname[] = "sest";
const int pgPort = 5432; // Porta padrão para PostgreSQL

PGconnection conn(&client, 0, 1024, nullptr);

char inbuf[128];
int pg_status = 0;

// Set to read every 60 sec
unsigned long previousMillis = 0;
const long interval = 60000;

const int sensorPin = 4;
int sensorState = 0;

void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Connecting to: ");
    Serial.println(ssid);
  }
  Serial.println("Connected to WiFi");
  Serial.println("Timer set to 60 seconds.");
}

void doPg(void) {
  int rc;

  if (pg_status == 0) {
    Serial.println("Attempting connection...");

    // Configuração da conexão com PostgreSQL
    conn.setDbLogin(PGIP, user, password, dbname, "utf8", pgPort);
    pg_status = 1;
    return;
  }

  if (pg_status == 1) {
    Serial.print("Connection status: ");
    rc = conn.status();
    if (rc == CONNECTION_BAD || rc == CONNECTION_NEEDED) {
      Serial.print("Connection error: ");
      Serial.println(conn.getMessage());
      pg_status = -1;
      return;
    } else if (rc == CONNECTION_OK) {
      pg_status = 2;
      Serial.println("Connection established.");
    }
    return;
  }

  if (pg_status == 2 && strlen(inbuf) > 0) {
    if (conn.execute(inbuf)) {
      Serial.println("Error executing query.");
      pg_status = -1;
      memset(inbuf, 0, sizeof(inbuf));
      return;
    }
    Serial.println("Query executed.");
    pg_status = 3;
    memset(inbuf, 0, sizeof(inbuf));
  }

  if (pg_status == 3) {
    rc = conn.getData();
    if (rc < 0) {
      Serial.println("Error getting data.");
      pg_status = -1;
      return;
    }
    if (rc == PG_RSTAT_READY) {
      pg_status = 2;
      Serial.println("Waiting for next query.");
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    int level_test = 0;

    if (WiFi.status() == WL_CONNECTED) {
      sensorState = digitalRead(sensorPin);

      if (sensorState != HIGH) {
        level_test += 1;
        Serial.print("Diesel level: ");
        Serial.println(level_test);

        // Prepare SQL query
        snprintf(inbuf, sizeof(inbuf), "INSERT INTO manutencao.diesel (nivel, unidade_dass) VALUES (%.2f, %d)", level_test, 1);
      }
    } else {
      Serial.println("WiFi Disconnected");
    }
  }

  delay(50); // Adjust delay as needed
  doPg(); // Call doPg() to handle PostgreSQL operations
}
