#include <Bonezegei_PN532.h>

#define SS_PIN  17
#define RST_PIN 5

Bonezegei_PN532 rfid(SS_PIN, RST_PIN);

// Default Key A (6 bytes)
uint8_t keyA[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t blockAddr = 4; // Block 4 (Sector 1)

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!rfid.begin()) {
    Serial.println("PN532 Initialization Failed!");
    while (1);
  }
  Serial.println("Scan card to write and read block data...");
}

void loop() {
  if (rfid.readCard()) {
    uint8_t uid[10];
    rfid.getUID(uid);

    Serial.print("Card detected: ");
    Serial.println(rfid.getUIDString());

    // 1. Authenticate with Key A
    if (rfid.authenticate(PICC_AUTHENT1A, blockAddr, keyA, uid)) {
      Serial.println("Authentication Successful!");

      // Data buffer must be exactly 16 bytes
      uint8_t writeData[16] = "Bonezegei_PN532!";

      // 2. Write Data
      if (rfid.writeBlock(blockAddr, writeData)) {
        Serial.println("Data successfully written to Block 4!");
      } else {
        Serial.println("Write failed!");
      }

      // 3. Read Data Back
      uint8_t readBuffer[16];
      if (rfid.readBlock(blockAddr, readBuffer)) {
        Serial.print("Read Block Data (Hex): ");
        for (int i = 0; i < 16; i++) {
          if (readBuffer[i] < 0x10) Serial.print("0");
          Serial.print(readBuffer[i], HEX);
          Serial.print(" ");
        }
        Serial.println();

        Serial.print("Read Block Data (Text): ");
        for (int i = 0; i < 16; i++) {
          Serial.print((char)readBuffer[i]);
        }
        Serial.println();
      } else {
        Serial.println("Read failed!");
      }
    } else {
      Serial.println("Authentication Failed!");
    }

    rfid.halt();
    rfid.stopCrypto();
    delay(2000);
  }
}