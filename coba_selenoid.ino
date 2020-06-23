#include <WiFi.h>
#include <MQTT.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HCSR04.h>


#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif



#define timeSeconds 10

const char ssid[] = "BERSINAR";
const char pass[] = "tanyajeki";

const int RST_pin = 22; // Reset pin
const int SS_pin = 21; // Slave select pin //MISO
const int relayPin = 15;
const int sound = 32;

#define bazzer  34
String Sensor = "System mati";
String alat;
int count;
int tepuk=0;
long rangeStart=0;
long range=0;
boolean statusPintu = false;

MFRC522 mfrc522(SS_pin, RST_pin); // Create MFRC522 instance
//
WiFiClient net;
MQTTClient client;

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("Umbu", "UmbuMade", "71170184")) { //client_id, username, password
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");
}


void rfid() {
  String uid;
  String temp;
  for (int i = 0; i < 4; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      temp = "0" + String(mfrc522.uid.uidByte[i], HEX);
    }
    else temp = String(mfrc522.uid.uidByte[i], HEX);

    if (i == 3) {
      uid =  uid + temp;
    }
    else uid =  uid + temp + " ";
  }
  Serial.println("UID " + uid);
  String Access = "37 f8 51 34"; //Akses RFID yang ditunjuk
  Access.toLowerCase();
  if (uid == Access) {
    if (Sensor == "System mati") {
      Serial.println("System nyala");
      client.publish("rfid", "System nyala");
      Sensor = "System nyala";
      count=0;
      alarmMati();
    }
    else {
      Serial.println("System mati");
      client.publish("rfid", "System mati");
      Sensor = "System mati";
      count=0;
      alarmMati();
    }
  }
  else {
    Serial.println("kartu tidak di kenal");
    count++;
    if(count >=3){
      alarm();
    }
  }
  Serial.println("\n");
  mfrc522.PICC_HaltA();
}

void sensorSuara(){
  float Analog = analogRead(sound);
  if(Analog>800){
    if(tepuk == 0){
      range = millis();
      rangeStart=range;
      tepuk++;
    }else if(tepuk > 0 && millis() -range >=50){
      tepuk++;
    }
  }
  Serial.println(Analog);
  if(millis()-rangeStart >=400){
    if(tepuk==1){
      if(!statusPintu){
        statusPintu = true;
        digitalWrite(relayPin, HIGH);
      }else if (statusPintu){
        statusPintu=false;
        digitalWrite(relayPin,LOW);
      }
    }
    tepuk=0;
  }
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  if (Sensor == "System nyala") {
    if (topic == "btnAlat" && payload == "1") {
      client.publish("relay", "Terbuka");
      digitalWrite(relayPin, HIGH);
    } else {
      client.publish("relay", "Tertutup");
      digitalWrite(relayPin, LOW);
    }
  }
}

void alarm(){
  digitalWrite(bazzer,HIGH);
  client.publish("pesan","ada penyusup");
  
}
void alarmMati(){
  digitalWrite(bazzer,LOW);
  client.publish("pesan","aman");
  
}
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  pinMode(relayPin, OUTPUT);
  pinMode(bazzer,OUTPUT);
  digitalWrite(relayPin, LOW);
  while (!Serial);
  SPI.begin();
  mfrc522.PCD_Init();
  mfrc522.PCD_DumpVersionToSerial();
  client.begin("broker.shiftr.io", net);
  client.onMessage(messageReceived);
  connect();
client.subscribe("btnAlat");
}

void loop() {
  client.loop();
  delay(500);

  if (!client.connected()) {
    connect();
  }

  if(Sensor == "System nyala"){
    
  sensorSuara();
  }
  //RFID
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  rfid();

}
