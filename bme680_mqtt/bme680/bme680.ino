#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
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

// ================= BME680 =================
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME680 bme;

// ================= MQTT CLIENT =================
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;

// ================= AES =================
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

        String clientId = "ESP32-BME680-";
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
    // Zwiekszony bufor do 512, poniewaz JSON z BME680 jest dluzszy niz UID z RFID
    unsigned char base64[512]; 

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

    // Inicjalizacja czujnika BME680 (Piny SDA: 21, SCL: 22)
    // Jesli czujnik nie odpowiada, zmien adres z 0x77 na 0x76
    if (!bme.begin(0x77)) {
        Serial.println("Nie znaleziono czujnika BME680! Sprawdz piny.");
        while (1);
    }

    // Konfiguracja BME680
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320°C przez 150 ms

    Serial.println("BME680 gotowy");

    connectWiFi();
    client.setServer(mqtt_server, mqtt_port);
}

void loop() {
    if (!client.connected())
        reconnectMQTT();

    client.loop();

    // Pętla czasowa wysyłająca dane co 5000 ms (5 sekund)
    unsigned long now = millis();
    if (now - lastMsg > 5000) {
        lastMsg = now;

        if (!bme.performReading()) {
            Serial.println("Blad odczytu z czujnika BME680");
            return;
        }

        // Budowanie paczki danych JSON z odczytami z czujnika
        String payload = "{\"deviceId\":\"esp-bme680-01\",";
        payload += "\"temp\":" + String(bme.temperature, 2) + ",";
        payload += "\"hum\":" + String(bme.humidity, 2) + ",";
        payload += "\"press\":" + String(bme.pressure / 100.0, 2) + ",";
        payload += "\"gas\":" + String(bme.gas_resistance / 1000.0, 2) + "}";

        Serial.println("JSON:");
        Serial.println(payload);

        // Szyfrowanie JSON-a za pomoca AES-256 CBC + Base64
        String encrypted = encrypt(payload);

        Serial.println("Encrypted (Base64):");
        Serial.println(encrypted);

        // Wysylka na nowy temat dedykowany dla BME680
        if (client.publish("iot/bme680", encrypted.c_str())) {
            Serial.println("MQTT wyslane");
        } else {
            Serial.println("Blad MQTT");
        }
        Serial.println("------------------------------------");
    }
}