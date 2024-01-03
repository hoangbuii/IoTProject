//Include Library
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WIFI and MQTT Broker information
const char* ssid = "POCO X4 GT";
const char* password = "thuongcute";
const char* mqtt_server = "13.231.154.153";

// PIN connection
#define DHTTYPE DHT11
#define DHT_PIN D2
#define SOIL_MOISTURE_PIN A0
#define LIGHT1_PIN D3
#define LIGHT2_PIN D4
#define PUMP_PIN D5
#define AIR_PIN D6

// Necessary Variable
DHT dht(DHT_PIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
float temp = 0;
float humid = 0;
int soil = 0;
int light1_st = 0;
int light2_st = 0;
int air_st = 0;
int pump_st = 0;
char tmp[10];

// Setup WIFI connection
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Process when received control data
// Check topic and control actors
void processData(char* topic, byte* payload) {
  if (strcmp(topic, "light1") == 0) {
    if((char)payload[0] == '1') {
      digitalWrite(LIGHT1_PIN, HIGH);
      light1_st = 1;
    } else {
      digitalWrite(LIGHT1_PIN, LOW);
      light1_st = 0;
    }
    dtostrf(light1_st, 1, 0, tmp);
    snprintf (msg, MSG_BUFFER_SIZE, tmp);
    client.publish("light1-st", msg);
  } else if (strcmp(topic, "light2") == 0) {
    if((char)payload[0] == '1') {
      digitalWrite(LIGHT2_PIN, HIGH);
      light2_st = 1;
    } else {
      digitalWrite(LIGHT2_PIN, LOW);
      light2_st = 0;
    }
    dtostrf(light2_st, 1, 0, tmp);
    snprintf (msg, MSG_BUFFER_SIZE, tmp);
    client.publish("light2-st", msg);
  } else if (strcmp(topic, "air") == 0) {
    if((char)payload[0] == '1') {
      digitalWrite(AIR_PIN, HIGH);
      air_st = 1;
    } else {
      digitalWrite(AIR_PIN, LOW);
      air_st = 0;
    }
    dtostrf(air_st, 1, 0, tmp);
    snprintf (msg, MSG_BUFFER_SIZE, tmp);
    client.publish("air-st", msg);
  } else if (strcmp(topic, "pump") == 0) {
    if((char)payload[0] == '1') {
      digitalWrite(PUMP_PIN, HIGH);
      pump_st = 1;
    } else {
      digitalWrite(PUMP_PIN, LOW);
      pump_st = 0;
    }
    dtostrf(pump_st, 1, 0, tmp);
    snprintf (msg, MSG_BUFFER_SIZE, tmp);
    client.publish("pump-st", msg);
  }
}

//Get data from MQTT broker
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  processData(topic, payload);
  Serial.println();
}

// Connect and reconnect to WIFI and MQTT broker
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // ... and resubscribe
      client.subscribe("light1");
      client.subscribe("light2");
      client.subscribe("air");
      client.subscribe("pump");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// Read sensordata from Sensor and publish it to MQTT broker
void publishSensorData() {
  temp = dht.readTemperature();
  humid = dht.readHumidity();
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  soil = map(soilMoistureValue, 1023, 0, 0, 100);


  dtostrf(temp, 4, 3, tmp);
  snprintf (msg, MSG_BUFFER_SIZE, tmp);
  client.publish("temp", msg);
  dtostrf(humid, 4, 1, tmp);
  snprintf (msg, MSG_BUFFER_SIZE, tmp);
  client.publish("hum", msg);
  dtostrf(soil, 4, 0, tmp);
  snprintf (msg, MSG_BUFFER_SIZE, tmp);
  client.publish("soilMoisture", msg);
  
 client.publish("sql1", msg);
}


void setup() {
  // put your setup code here, to run once:
  setup_wifi();
  Serial.begin(9600);
  dht.begin();
  pinMode(AIR_PIN, OUTPUT);
  pinMode(LIGHT1_PIN, OUTPUT);
  pinMode(LIGHT2_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 30000) {
    lastMsg = now;
    publishSensorData();
  }

  if (temp > 30 && air_st == 0) {
    digitalWrite(AIR_PIN, HIGH);
    air_st = 1;
    dtostrf(air_st, 1, 0, tmp);
    snprintf (msg, MSG_BUFFER_SIZE, tmp);
    client.publish("air-st", msg);
  }
  if (temp < 25 && air_st == 1) {
    digitalWrite(AIR_PIN, LOW);
    air_st = 0;
    dtostrf(air_st, 1, 0, tmp);
    snprintf (msg, MSG_BUFFER_SIZE, tmp);
    client.publish("air-st", msg);
  }

  if (soil < 10 && pump_st == 0) {
    digitalWrite(PUMP_PIN, HIGH);
    pump_st = 1;
    dtostrf(pump_st, 1, 0, tmp);
    snprintf (msg, MSG_BUFFER_SIZE, tmp);
    client.publish("pump-st", msg);
  }
  if (soil > 50 && pump_st == 1) {
    digitalWrite(PUMP_PIN, LOW);
    pump_st = 0;
    dtostrf(pump_st, 1, 0, tmp);
    snprintf (msg, MSG_BUFFER_SIZE, tmp);
    client.publish("pump-st", msg);
  }
}
