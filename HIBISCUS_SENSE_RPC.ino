#include <WiFi.h>
#include <MQTT.h>
#include <Adafruit_APDS9960.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_NeoPixel.h>
#include <SimpleDHT.h>

Adafruit_NeoPixel rgb(1, 16);

Adafruit_APDS9960 apds;
Adafruit_BME280 bme;
Adafruit_MPU6050 mpu;

sensors_event_t a, g, temp;

SimpleDHT11 dht(15);

const char ssid[] = "YOUR_WIFI_SSID";
const char pass[] = "YOUR_WIFI_PASSWORD";

const char device_developer_id[] = "deviceDefault@favoriot";
const char device_access_token[] = "rw-apikey/accesstoken";

const char publish_topic[]  = "YOUR_DEVICE_ACCESS_TOKEN/v2/streams";
const char rpc_topic[]      = "YOUR_DEVICE_ACCESS_TOKEN/v2/rpc";

WiFiClient net;
MQTTClient client(1024);

unsigned long lastMillis = 0;

void connect() {
  Serial.print("Connecting to  WiFi ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.print(" connected.\nConnecting to Favoriot MQTT Broker ");
  while (!client.connect("YOUR_DEVICE_MQTT_CLIENT_ID", device_access_token, device_access_token)) {
    Serial.print(".");
    delay(100);
  }

  client.subscribe(rpc_topic);

  Serial.println(" connected.");

  Serial.println("    ______                       _       __ ");
  Serial.println("   / ____/___ __   ______  _____(_)___  / /_");
  Serial.println("  / /_  / __ `/ | / / __ \\/ ___/ / __ \\/ __/");
  Serial.println(" / __/ / /_/ /| |/ / /_/ / /  / / /_/ / /_  ");
  Serial.println("/_/    \\__,_/ |___/\\____/_/  /_/\\____/\\__/");
  Serial.println("Favoriot MQTT, Ready!\n");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  
  String input = payload.substring(payload.indexOf("{\"") + 2, payload.indexOf("\":\""));
  String state = payload.substring(payload.indexOf(":\"") + 2, payload.indexOf("\",\""));

  if(input == "sw") digitalWrite(2, state.toInt());
  if(input == "slider"){
    rgb.setPixelColor(0, state.toInt(), 0, 0);
    rgb.show();
  }
  
}

void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, pass);

  client.begin("mqtt.favoriot.com", net);
  client.onMessage(messageReceived);

  connect();

  pinMode(2, OUTPUT); digitalWrite(2, HIGH);

  rgb.begin();
  rgb.show();

  if (!apds.begin()){
    Serial.println("Failed to find Hibiscus Sense APDS9960 chip");
  }

  apds.enableProximity(true);

  if (!bme.begin()){
    Serial.println("Failed to find Hibiscus Sense BME280 chip");
  }

  if (!mpu.begin()){
    Serial.println("Failed to find Hibiscus Sense MPU6050 chip");
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

}

void loop() {
  client.loop();
  delay(10);

  if (!client.connected()) {
    connect();
  }

  byte temperature, humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    return;
  }

  if(millis() - lastMillis > 1500){

    lastMillis = millis();
    
    String json = "{\"device_developer_id\":\"" + String(device_developer_id) + "\",\"data\":{";
    
    json += "\"pro\":" + String(apds.readProximity()) + ",";
    json += "\"alt\":" + String(bme.readAltitude(1013.25)) + ",";
    json += "\"bar\":" + String(bme.readPressure()/100.00) + ",";
    json += "\"otm\":" + String(bme.readTemperature()) + ",";
    json += "\"tem\":" + String(temperature) + ",";
    json += "\"hum\":" + String(humidity) + ",";
    
    mpu.getEvent(&a,&g,&temp);
    
    json += "\"acx\":" + String(a.acceleration.x) + ",";
    json += "\"acy\":" + String(a.acceleration.y) + ",";
    json += "\"acz\":" + String(a.acceleration.z) + ",";
    json += "\"grx\":" + String(g.gyro.x) + ",";
    json += "\"gry\":" + String(g.gyro.y) + ",";
    json += "\"grz\":" + String(g.gyro.z) + "";
    
    json += "}}";

    client.publish(publish_topic, json);

    Serial.println(json);
  }
}
