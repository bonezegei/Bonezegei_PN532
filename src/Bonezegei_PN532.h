/*
    PN532 RFID/NFC Reader/Writer with MIFARE Classic
    Supports SPI, I2C, and Hardware Serial (UART)
    Author: Jofel Batutay (Bonezegei)
    Date: November 3, 2026
*/

#ifndef BONEZEGEI_PN532_H
#define BONEZEGEI_PN532_H

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

// Interface Selection Modes
enum PN532_Interface {
  PN532_SPI_MODE,
  PN532_I2C_MODE,
  PN532_UART_MODE
};

// Protocol Framing
#define PN532_PREAMBLE                      (0x00)
#define PN532_STARTCODE1                    (0x00)
#define PN532_STARTCODE2                    (0xFF)
#define PN532_POSTAMBLE                     (0x00)

#define PN532_HOSTTOPN532                   (0xD4)
#define PN532_PN532TOHOST                   (0xD5)

// PN532 Commands
#define PN532_COMMAND_GETFIRMWAREVERSION    (0x02)
#define PN532_COMMAND_SAMCONFIGURATION      (0x14)
#define PN532_COMMAND_INLISTPASSIVETARGET   (0x4A)
#define PN532_COMMAND_INDATAEXCHANGE        (0x40)

// PICC Commands
#define PICC_AUTHENT1A                      (0x60)
#define PICC_AUTHENT1B                      (0x61)
#define PICC_READ                           (0x30)
#define PICC_WRITE                          (0xA0)
#define PICC_HALT                           (0x50)

// Status Codes
#define MI_OK                               (0)
#define MI_NOTAGERR                         (1)
#define MI_ERR                              (2)

class Bonezegei_PN532 {
  public:
    // SPI Constructors
    Bonezegei_PN532(uint8_t ssPin, uint8_t rstPin = 0xFF);
    Bonezegei_PN532(SPIClass &spi, uint8_t ssPin, uint8_t rstPin = 0xFF);

    // I2C Constructor
    Bonezegei_PN532(TwoWire &wire, uint8_t rstPin = 0xFF, uint8_t i2cAddr = 0x24);

    // UART Constructor
    Bonezegei_PN532(HardwareSerial &serial, uint8_t rstPin = 0xFF);

    bool begin();
    void reset();
    void wakeup();

    bool isCard();
    bool readCard();

    uint8_t getUIDSize();
    void getUID(uint8_t *buffer);
    String getUIDString();

    // MIFARE Data & Auth Functions
    bool authenticate(uint8_t authMode, uint8_t blockAddr, uint8_t *key, uint8_t *uid);
    bool readBlock(uint8_t blockAddr, uint8_t *buffer);
    bool writeBlock(uint8_t blockAddr, uint8_t *buffer);
    void stopCrypto();
    void halt();

    // Utility & Diagnostics
    uint32_t getFirmwareVersion();
    bool SAMConfig();

  private:
    PN532_Interface _interface;

    SPIClass *_spi;
    TwoWire *_wire;
    HardwareSerial *_serial;

    uint8_t _ssPin;
    uint8_t _rstPin;
    uint8_t _i2cAddr;

    uint8_t _uid[10];
    uint8_t _uidSize;
    uint8_t _inListedTg;

    // Low Level Protocols
    bool writeCommand(uint8_t *cmd, uint8_t cmdLen, uint8_t *headerBuf = NULL, uint8_t headerLen = 0);
    int16_t readResponse(uint8_t *buf, uint8_t maxLen, uint16_t timeout = 1000);
    bool readAck(uint16_t timeout = 1000);
    bool isReady();
    bool waitReady(uint16_t timeout = 1000);
};

#endif