#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>
// #include <AESLib.h>
#include "mbedtls/aes.h"
#include "mbedtls/base64.h"


// ================= WIFI =================
const char* ssid = "michjuxgoonspot";
const char* password = "valuta1234";

// ================= MQTT =================
const char* mqtt_server = "10.63.142.119";
const int mqtt_port = 1883;

const char* mqtt_user = "esp32_rfid";
const char* mqtt_password = "password";

// ================= RFID =================
#define SS_PIN   27
#define RST_PIN  32

#define SCK_PIN  26
#define MISO_PIN 33
#define MOSI_PIN 25

MFRC522 rfid(SS_PIN, RST_PIN);

// ================= MQTT =================
WiFiClient espClient;
PubSubClient client(espClient);

// ================= AES =================
// AESLib aesLib;

// 32 bajty = AES-256
byte aesKey[] = {
  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
  0x38,0x39,0x41,0x42,0x43,0x44,0x45,0x46,
  0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
  0x38,0x39,0x41,0x42,0x43,0x44,0x45,0x46
};

// 16 bajtów IV
byte aesIv[] = {
  0x41,0x42,0x43,0x44,
  0x45,0x46,0x47,0x48,
  0x49,0x4A,0x4B,0x4C,
  0x4D,0x4E,0x4F,0x50
};

void connectWiFi() {

    Serial.print("Laczenie z WiFi");

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi OK");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

void reconnectMQTT() {

    while (!client.connected()) {

        Serial.print("Laczenie z MQTT...");

        String clientId = "ESP32-RFID-";
        clientId += String(random(0xffff), HEX);

        if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {

            Serial.println(" OK");

        } else {

            Serial.print(" blad=");
            Serial.println(client.state());

            delay(5000);
        }
    }
}

String getUID() {

    String uid = "";

    for (byte i = 0; i < rfid.uid.size; i++) {

        if (rfid.uid.uidByte[i] < 0x10)
            uid += "0";

        uid += String(rfid.uid.uidByte[i], HEX);
    }

    uid.toUpperCase();

    return uid;
}

String encrypt(String message)
{
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    unsigned char iv[16];
    memcpy(iv, aesIv, 16);

    int len = message.length();

    // PKCS#7 padding
    int paddedLen = ((len / 16) + 1) * 16;

    unsigned char input[paddedLen];
    memset(input, paddedLen - len, paddedLen);
    memcpy(input, message.c_str(), len);

    unsigned char output[paddedLen];

    mbedtls_aes_setkey_enc(&aes, aesKey, 256);

    mbedtls_aes_crypt_cbc(
        &aes,
        MBEDTLS_AES_ENCRYPT,
        paddedLen,
        iv,
        input,
        output
    );

    mbedtls_aes_free(&aes);

    size_t base64Len = 0;
    unsigned char base64[256];

    mbedtls_base64_encode(
        base64,
        sizeof(base64),
        &base64Len,
        output,
        paddedLen
    );

    base64[base64Len] = '\0';

    return String((char*)base64);
}

void setup() {

    Serial.begin(115200);

    randomSeed(micros());

    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);

    rfid.PCD_Init();

    Serial.println("RFID gotowy");

    connectWiFi();

    client.setServer(mqtt_server, mqtt_port);
}

void loop() {

    if (!client.connected())
        reconnectMQTT();

    client.loop();

    if (!rfid.PICC_IsNewCardPresent())
        return;

    if (!rfid.PICC_ReadCardSerial())
        return;

    String uid = getUID();

    Serial.print("UID: ");
    Serial.println(uid);

    String payload =
        "{\"deviceId\":\"esp-rfid-01\",\"uid\":\"" + uid + "\"}";

    Serial.println("JSON:");
    Serial.println(payload);

    String encrypted = encrypt(payload);

    Serial.println("Encrypted:");
    Serial.println(encrypted);

    if (client.publish("iot/rfid", encrypted.c_str())) {
        Serial.println("MQTT wyslane");
    } else {
        Serial.println("Blad MQTT");
    }

    rfid.PICC_HaltA();

    delay(1000);
}