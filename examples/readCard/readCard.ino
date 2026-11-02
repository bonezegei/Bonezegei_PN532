#include <Bonezegei_PN532.h>

// --- SELECT INTERFACE MODE ---
// Option 1: SPI Mode
#define SS_PIN  17
#define RST_PIN 5
Bonezegei_PN532 rfid(SS_PIN, RST_PIN);

// Option 2: I2C Mode (Uncomment to use)
// Bonezegei_PN532 rfid(Wire, 5 /* RST_PIN */, 0x24 /* I2C Addr */);

// Option 3: UART/HardwareSerial Mode (Uncomment to use)
// Bonezegei_PN532 rfid(Serial1, 5 /* RST_PIN */);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Initializing Bonezegei PN532...");
  if (rfid.begin()) {
    Serial.println("PN532 Ready!");
    Serial.print("Firmware Version: 0x");
    Serial.println(rfid.getFirmwareVersion(), HEX);
  } else {
    Serial.println("PN532 Initialization Failed!");
  }
}

void loop() {
  if (rfid.readCard()) {
    Serial.print("Card UID: ");
    Serial.println(rfid.getUIDString());
    rfid.halt();
    rfid.stopCrypto();
    delay(1000);
  }
}