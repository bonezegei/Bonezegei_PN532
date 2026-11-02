#include <Bonezegei_PN532.h>

#define SS_PIN  17
#define RST_PIN 5

Bonezegei_PN532 rfid(SS_PIN, RST_PIN);

uint8_t defaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Storage for cloned 1K MIFARE Classic (64 blocks x 16 bytes)
uint8_t clonedMemory[64][16];
bool cardDumped = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!rfid.begin()) {
    Serial.println("PN532 Init Failed!");
    while (1);
  }

  Serial.println("=== MIFARE Classic Card Cloner ===");
  Serial.println("STEP 1: Place ORIGINAL card to read data into memory...");
}

void loop() {
  if (!rfid.readCard()) return;

  uint8_t uid[10];
  rfid.getUID(uid);

  if (!cardDumped) {
    // --- READ / DUMP SOURCE CARD ---
    Serial.print("Source Card Detected UID: ");
    Serial.println(rfid.getUIDString());

    bool dumpOk = true;
    for (uint8_t block = 0; block < 64; block++) {
      // Authenticate each sector before reading blocks
      if (block % 4 == 0) {
        if (!rfid.authenticate(PICC_AUTHENT1A, block, defaultKey, uid)) {
          Serial.print("Auth failed at block ");
          Serial.println(block);
          dumpOk = false;
          break;
        }
      }

      if (!rfid.readBlock(block, clonedMemory[block])) {
        Serial.print("Read error at block ");
        Serial.println(block);
        dumpOk = false;
        break;
      }
    }

    rfid.halt();
    rfid.stopCrypto();

    if (dumpOk) {
      cardDumped = true;
      Serial.println("\nSource Card Dumped Successfully!");
      Serial.println("STEP 2: Remove source card and place TARGET (CUID/Magic) card to clone...");
    } else {
      Serial.println("Dump failed! Retrying...");
    }

    delay(2000);

  } else {
    // --- WRITE TO TARGET (CUID / MAGIC CARD) ---
    Serial.print("Target Card Detected UID: ");
    Serial.println(rfid.getUIDString());

    bool writeOk = true;

    for (uint8_t block = 0; block < 64; block++) {
      // Re-authenticate sector
      if (block % 4 == 0) {
        if (!rfid.authenticate(PICC_AUTHENT1A, block, defaultKey, uid)) {
          Serial.print("Target Auth failed at block ");
          Serial.println(block);
          writeOk = false;
          break;
        }
      }

      // Write block data (including Block 0 for CUID tags)
      if (!rfid.writeBlock(block, clonedMemory[block])) {
        Serial.print("Write error at block ");
        Serial.println(block);
        writeOk = false;
        break;
      }
    }

    rfid.halt();
    rfid.stopCrypto();

    if (writeOk) {
      Serial.println("\n*** CARD CLONED SUCCESSFULLY! ***");
      cardDumped = false; // Reset state for next operation
      Serial.println("\nPlace a new ORIGINAL card to clone another...");
    } else {
      Serial.println("Cloning failed! Make sure target card is CUID/Gen2 writeable.");
    }

    delay(3000);
  }
}