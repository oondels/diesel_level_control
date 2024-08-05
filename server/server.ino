#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <SimplePgSQL.h>
#include <Ultrasonic.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Configuração do sensor
#define TRIGGER_PIN  13
#define ECHO_PIN     5 

Ultrasonic ultrasonic(TRIGGER_PIN, ECHO_PIN);

// Configuração da rede WiFi
const char *ssid = "Drogo";
const char *pass = "75982466703";
WiFiClient client;

// Configuração do banco de dados
IPAddress PGIP(192, 168, 1, 17);
const char user[] = "postgres";
const char password[] = "wa0i4Ochu";
const char dbname[] = "sest";
const int pgPort = 5432;
PGconnection conn(&client, 0, 1024, nullptr);
char inbuf[128];
int pg_status = 0;

// Intervalo de leitura (10 segundos)
unsigned long previousMillis = 0;
const long interval = 20000;


float measureVolume(float distance_from_sensor, float r) {
  // Dados do cilindro
  float h_total = 22.3; // Altura total do cilindro em centímetros
  
  // Calcula a altura preenchida
  float h_filled = h_total - distance_from_sensor;

  // Calcula o volume atual preenchido
  float v_filled = M_PI * pow(r, 2) * h_filled;

  float v_total = M_PI * pow(r, 2) * h_total;

  return v_filled; // Volume preenchido em cm³
}



void setup() {
  Serial.begin(115200);
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  //pinMode(LED_BUILTIN, OUTPUT);

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Connecting to: ");
    Serial.println(ssid);
  }
  Serial.println("Connected to WiFi");
  Serial.println("Timer set to 10 seconds.");
}

void doPg(void) {
  char *msg;
  int rc;

  if (!pg_status) {
    Serial.println("Attempting connection...");

    // Configuração da conexão com PostgreSQL
    conn.setDbLogin(PGIP, user, password, dbname, "utf8", pgPort);
    pg_status = 1;
    return;
  }

  if (pg_status == 1) {
    rc = conn.status();
    Serial.print("Connection status: ");
    Serial.println(pg_status);

    if (rc == CONNECTION_BAD || rc == CONNECTION_NEEDED) {
      char *c = conn.getMessage();
      if (c) Serial.println(c);
      pg_status = -1;
      Serial.print("Connection error: ");
      Serial.println(conn.getMessage());
    } else if (rc == CONNECTION_OK) {
      pg_status = 2;
      Serial.println("Connection established.");
      Serial.println("Starting query");
    }
    return;
  }

  if (pg_status == 2 && strlen(inbuf) > 0) {
    if (conn.execute(inbuf)) goto error;
    Serial.println("Working...");
    pg_status = 3;
    memset(inbuf, 0, sizeof(inbuf));
  }

  if (pg_status == 3) {
    rc = conn.getData();
    int i;
    if (rc < 0) goto error;
    if (!rc) return;
    if (rc & PG_RSTAT_HAVE_COLUMNS) {
      for (i = 0; i < conn.nfields(); i++) {
        if (i) Serial.print(" | ");
        Serial.print(conn.getColumn(i));
      }
      Serial.println("\n==========");
    }
    else if (rc & PG_RSTAT_HAVE_ROW) {
      for (i = 0; i < conn.nfields(); i++) {
        if (i) Serial.print(" | ");
        msg = conn.getValue(i);
        if (!msg) msg = (char *)"NULL";
        Serial.print(msg);
      }
      Serial.println();
    }
    else if (rc & PG_RSTAT_HAVE_SUMMARY) {
      Serial.print("Rows affected: ");
      Serial.println(conn.ntuples());
    }
    else if (rc & PG_RSTAT_HAVE_MESSAGE) {
      msg = conn.getMessage();
      if (msg) Serial.println(msg);
    }
    if (rc & PG_RSTAT_READY) {
      pg_status = 2;
      Serial.println("Waiting query");
    }
  }
  return;
  
  error:
  msg = conn.getMessage();
  if (msg) Serial.println(msg);
  else Serial.println("UNKNOWN ERROR");
  if (conn.status() == CONNECTION_BAD) {
    Serial.println("Connection is bad");
    pg_status = -1;
  }
}

void loop() {

  delay(50); 
  doPg();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    digitalWrite(LED_BUILTIN, HIGH);
    delay(500); 
    digitalWrite(LED_BUILTIN, LOW);
    if (WiFi.status() == WL_CONNECTED) {

      digitalWrite(TRIGGER_PIN, LOW);
      delayMicroseconds(2);
      digitalWrite(TRIGGER_PIN, HIGH);
      delayMicroseconds(10);
      digitalWrite(TRIGGER_PIN, LOW);

      long duration = pulseIn(ECHO_PIN, HIGH);
      float distance = duration * 0.0343 / 2;
      float volume = measureVolume(distance, 3.85);

      if (distance == 0) {
        Serial.println("Erro no sensor");
        } else {
          Serial.print("Distance: ");
          Serial.print(distance);
          Serial.println(" cm");

          Serial.println("=============================");
          
          Serial.print("Volume: ");
          Serial.print(volume);
          Serial.println(" ml");
        }

      delay(1000);
      // Prepare SQL query
      snprintf(inbuf, sizeof(inbuf), "INSERT INTO manutencao.diesel (nivel, unidade_dass) VALUES (%.2f, %d)", volume, 1);   

      
    } else {
      Serial.println("WiFi Disconnected");
    }
  }
}


  
