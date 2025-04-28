/**
 * R200 UHF RFID Reader with MQTT
 * Adapted from Playful Technology's R200 example
 */

 #include <WiFi.h>
 #include <PubSubClient.h>
 #include "R200.h"
 
 // WiFi Configuration
 const char* SSID = "Mr ASH";
 const char* PASSWORD = "island@36";
 
 // MQTT Configuration
 const char* BROKER = "test.mosquitto.org";
 const int BROKER_PORT = 1883;
 const char* CLIENT_ID = "ESP32_RFID_001";
 
 // RFID Topics
 const char* RFID_SCAN_TOPIC = "rfid-Velociti/scans";
 const char* RFID_STATUS_TOPIC = "rfid-Velociti/status";
 
 // Hardware Pins
 #define BUZZER_PIN 32
 #define RFID_RX_PIN 16  // R200 RX pin on ESP32
 #define RFID_TX_PIN 17  // R200 TX pin on ESP32
 
 // Global variables
 R200 rfid;
 WiFiClient espClient;
 PubSubClient client(espClient);
 unsigned long lastPollTime = 0;
 uint8_t lastUID[12] = {0};
 bool cardWasPresent = false;
 
 void setup_wifi() {
   //delay(10);
   Serial.println();
   Serial.print("Connecting to ");
   Serial.println(SSID);
 
   WiFi.begin(SSID, PASSWORD);
 
   while (WiFi.status() != WL_CONNECTED) {
     //delay(500);
     Serial.print(".");
   }
 
   Serial.println("");
   Serial.println("WiFi connected");
   Serial.println("IP address: ");
   Serial.println(WiFi.localIP());
 }
 
 void reconnect_mqtt() {
   while (!client.connected()) {
     Serial.print("Attempting MQTT connection...");
     if (client.connect(CLIENT_ID)) {
       Serial.println("connected");
       client.publish(RFID_STATUS_TOPIC, "Reader Online");
     } else {
       Serial.print("failed, rc=");
       Serial.print(client.state());
       Serial.println(" retrying in 5 seconds");
       //delay(5000);
     }
   }
 }
 
 
 
 String uidToHexString(uint8_t* uid, int length) {
   String hexString = "";
   for (int i = 0; i < length; i++) {
     if (uid[i] < 0x10) {
       hexString += "0";
     }
     hexString += String(uid[i], HEX);
   }
   hexString.toUpperCase(); // This modifies the string in place
   return hexString; // Return the modified string
 }
 
 bool isNewCard() {
   // Check if a card is present (non-zero UID)
   bool cardIsPresent = false;
   for (int i = 0; i < 12; i++) {
     if (rfid.uid[i] != 0) {
       cardIsPresent = true;
       break;
     }
   }
   
   // Check if it's a new card
   if (cardIsPresent && !cardWasPresent) {
     cardWasPresent = true;
     return true;
   } else if (!cardIsPresent) {
     cardWasPresent = false;
   }
   return false;
 }
 
 void handleRfidScan() {
   if (isNewCard()) {
     // Convert the UID to a hex string
     String uidStr = uidToHexString(rfid.uid, 12);
     
     // Create JSON payload
     String payload = "{\"uid\":\"" + uidStr + "\",\"reader_id\":\"" + String(CLIENT_ID) + "\"}";
     
     if (client.publish(RFID_SCAN_TOPIC, payload.c_str())) {
       Serial.print("Published: ");
       Serial.println(payload);
     } else {
       Serial.println("Publish failed");
     }
   }
 }
 
 void setup() {
   Serial.begin(115200);
   
   // Initialize R200 RFID reader
   rfid.begin(&Serial2, 115200, RFID_RX_PIN, RFID_TX_PIN);
   //Serial.println("R200 RFID reader initialized");
   
   // Initialize Buzzer
  //  pinMode(BUZZER_PIN, OUTPUT);
  //  digitalWrite(BUZZER_PIN, LOW);
   
   // Connect to WiFi
   setup_wifi();
   
   // Configure MQTT client
   client.setServer(BROKER, BROKER_PORT);
 }
 
 void loop() {
   // Handle MQTT connection
   if (!client.connected()) {
     reconnect_mqtt();
   }
   client.loop();
   
   // Handle the R200 RFID reader
   rfid.loop(); // Process any incoming data from the reader
   
   // Poll for cards every second
   if (millis() - lastPollTime > 10) {
     rfid.poll();
     lastPollTime = millis();
   }
   
   // Check for card scans and publish
   handleRfidScan();
   
   //delay(100);
 }