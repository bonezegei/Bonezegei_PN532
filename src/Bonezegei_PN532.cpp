/*
    PN532 RFID/NFC Reader/Writer with MIFARE Classic
    Supports SPI, I2C, and Hardware Serial (UART)
    Author: Jofel Batutay (Bonezegei)
    Date: November 3, 2026
*/

#include "Bonezegei_PN532.h"

// SPI Constructors
Bonezegei_PN532::Bonezegei_PN532(uint8_t ssPin, uint8_t rstPin) {
  _interface = PN532_SPI_MODE;
  _spi = &SPI;
  _wire = NULL;
  _serial = NULL;
  _ssPin = ssPin;
  _rstPin = rstPin;
  _i2cAddr = 0x24;
  _uidSize = 0;
  _inListedTg = 1;
}

Bonezegei_PN532::Bonezegei_PN532(SPIClass &spi, uint8_t ssPin, uint8_t rstPin) {
  _interface = PN532_SPI_MODE;
  _spi = &spi;
  _wire = NULL;
  _serial = NULL;
  _ssPin = ssPin;
  _rstPin = rstPin;
  _i2cAddr = 0x24;
  _uidSize = 0;
  _inListedTg = 1;
}

// I2C Constructor
Bonezegei_PN532::Bonezegei_PN532(TwoWire &wire, uint8_t rstPin, uint8_t i2cAddr) {
  _interface = PN532_I2C_MODE;
  _spi = NULL;
  _wire = &wire;
  _serial = NULL;
  _ssPin = 0xFF;
  _rstPin = rstPin;
  _i2cAddr = i2cAddr;
  _uidSize = 0;
  _inListedTg = 1;
}

// UART Constructor
Bonezegei_PN532::Bonezegei_PN532(HardwareSerial &serial, uint8_t rstPin) {
  _interface = PN532_UART_MODE;
  _spi = NULL;
  _wire = NULL;
  _serial = &serial;
  _ssPin = 0xFF;
  _rstPin = rstPin;
  _i2cAddr = 0x24;
  _uidSize = 0;
  _inListedTg = 1;
}

bool Bonezegei_PN532::begin() {
  reset();

  if (_interface == PN532_SPI_MODE) {
    pinMode(_ssPin, OUTPUT);
    digitalWrite(_ssPin, HIGH);
    _spi->begin();
  } else if (_interface == PN532_I2C_MODE) {
    _wire->begin();
  } else if (_interface == PN532_UART_MODE) {
    _serial->begin(115200);
  }

  wakeup();

  uint32_t ver = getFirmwareVersion();
  if (ver == 0 || ver == 0xFFFFFFFF) {
    return false;
  }

  return SAMConfig();
}

void Bonezegei_PN532::reset() {
  if (_rstPin != 0xFF) {
    pinMode(_rstPin, OUTPUT);
    digitalWrite(_rstPin, HIGH);
    delay(10);
    digitalWrite(_rstPin, LOW);
    delay(50);
    digitalWrite(_rstPin, HIGH);
    delay(100);
  }
}

void Bonezegei_PN532::wakeup() {
  if (_interface == PN532_SPI_MODE) {
    digitalWrite(_ssPin, LOW);
    delay(2);
    digitalWrite(_ssPin, HIGH);
  } else if (_interface == PN532_UART_MODE) {
    uint8_t wakeupBuf[16] = {0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    _serial->write(wakeupBuf, 16);
    delay(10);
  } else if (_interface == PN532_I2C_MODE) {
    _wire->beginTransmission(_i2cAddr);
    _wire->endTransmission();
    delay(10);
  }
}

bool Bonezegei_PN532::isCard() {
  return readCard();
}

bool Bonezegei_PN532::readCard() {
  uint8_t cmd[2] = {0x01, 0x00}; // 1 target, 106 kbps ISO14443A
  uint8_t header = PN532_COMMAND_INLISTPASSIVETARGET;

  if (!writeCommand(cmd, 2, &header, 1)) return false;

  uint8_t buf[20];
  int16_t res = readResponse(buf, sizeof(buf));
  if (res < 7 || buf[0] != (PN532_COMMAND_INLISTPASSIVETARGET + 1)) return false;

  uint8_t numTags = buf[1];
  if (numTags == 0) return false;

  _inListedTg = buf[2];
  _uidSize = buf[6];

  for (uint8_t i = 0; i < _uidSize && i < 10; i++) {
    _uid[i] = buf[7 + i];
  }

  return true;
}

uint8_t Bonezegei_PN532::getUIDSize() {
  return _uidSize;
}

void Bonezegei_PN532::getUID(uint8_t *buffer) {
  for (uint8_t i = 0; i < _uidSize; i++) {
    buffer[i] = _uid[i];
  }
}

String Bonezegei_PN532::getUIDString() {
  String str = "";
  for (uint8_t i = 0; i < _uidSize; i++) {
    if (_uid[i] < 0x10) str += "0";
    str += String(_uid[i], HEX);
    if (i < _uidSize - 1) str += ":";
  }
  str.toUpperCase();
  return str;
}

bool Bonezegei_PN532::authenticate(uint8_t authMode, uint8_t blockAddr, uint8_t *key, uint8_t *uid) {
  uint8_t cmd[13];
  cmd[0] = _inListedTg;
  cmd[1] = authMode;
  cmd[2] = blockAddr;
  for (uint8_t i = 0; i < 6; i++) cmd[3 + i] = key[i];
  for (uint8_t i = 0; i < 4; i++) cmd[9 + i] = uid[i];

  uint8_t header = PN532_COMMAND_INDATAEXCHANGE;
  if (!writeCommand(cmd, 13, &header, 1)) return false;

  uint8_t buf[8];
  int16_t res = readResponse(buf, sizeof(buf));
  return (res >= 2 && buf[0] == (PN532_COMMAND_INDATAEXCHANGE + 1) && buf[1] == 0x00);
}

bool Bonezegei_PN532::readBlock(uint8_t blockAddr, uint8_t *buffer) {
  uint8_t cmd[3];
  cmd[0] = _inListedTg;
  cmd[1] = PICC_READ;
  cmd[2] = blockAddr;

  uint8_t header = PN532_COMMAND_INDATAEXCHANGE;
  if (!writeCommand(cmd, 3, &header, 1)) return false;

  uint8_t buf[20];
  int16_t res = readResponse(buf, sizeof(buf));
  if (res >= 18 && buf[0] == (PN532_COMMAND_INDATAEXCHANGE + 1) && buf[1] == 0x00) {
    for (uint8_t i = 0; i < 16; i++) {
      buffer[i] = buf[2 + i];
    }
    return true;
  }
  return false;
}

bool Bonezegei_PN532::writeBlock(uint8_t blockAddr, uint8_t *buffer) {
  uint8_t cmd[19];
  cmd[0] = _inListedTg;
  cmd[1] = PICC_WRITE;
  cmd[2] = blockAddr;
  for (uint8_t i = 0; i < 16; i++) {
    cmd[3 + i] = buffer[i];
  }

  uint8_t header = PN532_COMMAND_INDATAEXCHANGE;
  if (!writeCommand(cmd, 19, &header, 1)) return false;

  uint8_t buf[8];
  int16_t res = readResponse(buf, sizeof(buf));
  return (res >= 2 && buf[0] == (PN532_COMMAND_INDATAEXCHANGE + 1) && buf[1] == 0x00);
}

void Bonezegei_PN532::stopCrypto() {
  SAMConfig();
}

void Bonezegei_PN532::halt() {
  uint8_t cmd[2];
  cmd[0] = _inListedTg;
  cmd[1] = PICC_HALT;

  uint8_t header = PN532_COMMAND_INDATAEXCHANGE;
  writeCommand(cmd, 2, &header, 1);
}

uint32_t Bonezegei_PN532::getFirmwareVersion() {
  uint8_t header = PN532_COMMAND_GETFIRMWAREVERSION;
  if (!writeCommand(NULL, 0, &header, 1)) return 0;

  uint8_t buf[12];
  int16_t res = readResponse(buf, sizeof(buf));
  if (res < 5 || buf[0] != (PN532_COMMAND_GETFIRMWAREVERSION + 1)) return 0;

  uint32_t response = buf[1];
  response <<= 8;
  response |= buf[2];
  response <<= 8;
  response |= buf[3];
  response <<= 8;
  response |= buf[4];

  return response;
}

bool Bonezegei_PN532::SAMConfig() {
  uint8_t cmd[3] = {0x01, 0x14, 0x01};
  uint8_t header = PN532_COMMAND_SAMCONFIGURATION;

  if (!writeCommand(cmd, 3, &header, 1)) return false;

  uint8_t buf[8];
  int16_t res = readResponse(buf, sizeof(buf));
  return (res >= 1 && buf[0] == (PN532_COMMAND_SAMCONFIGURATION + 1));
}

bool Bonezegei_PN532::isReady() {
  if (_interface == PN532_SPI_MODE) {
    digitalWrite(_ssPin, LOW);
    _spi->beginTransaction(SPISettings(1000000, LSBFIRST, SPI_MODE0));
    _spi->transfer(0x02);
    uint8_t status = _spi->transfer(0x00);
    _spi->endTransaction();
    digitalWrite(_ssPin, HIGH);
    return (status & 0x01) == 0x01;
  } else if (_interface == PN532_I2C_MODE) {
    _wire->requestFrom(_i2cAddr, (uint8_t)1);
    if (_wire->available()) {
      uint8_t status = _wire->read();
      return (status & 0x01) == 0x01;
    }
    return false;
  } else if (_interface == PN532_UART_MODE) {
    return _serial->available() > 0;
  }
  return false;
}

bool Bonezegei_PN532::waitReady(uint16_t timeout) {
  uint32_t start = millis();
  while (millis() - start < timeout) {
    if (isReady()) return true;
    delay(1);
  }
  return false;
}

bool Bonezegei_PN532::writeCommand(uint8_t *cmd, uint8_t cmdLen, uint8_t *headerBuf, uint8_t headerLen) {
  uint8_t frameLen = headerLen + cmdLen + 1;
  uint8_t lcs = (~frameLen + 1) & 0xFF;

  uint8_t dcsSum = PN532_HOSTTOPN532;
  for (uint8_t i = 0; i < headerLen; i++) dcsSum += headerBuf[i];
  for (uint8_t i = 0; i < cmdLen; i++) dcsSum += cmd[i];
  uint8_t dcs = (~dcsSum + 1) & 0xFF;

  if (_interface == PN532_SPI_MODE) {
    digitalWrite(_ssPin, LOW);
    _spi->beginTransaction(SPISettings(1000000, LSBFIRST, SPI_MODE0));
    _spi->transfer(0x01);

    _spi->transfer(PN532_PREAMBLE);
    _spi->transfer(PN532_STARTCODE1);
    _spi->transfer(PN532_STARTCODE2);
    _spi->transfer(frameLen);
    _spi->transfer(lcs);
    _spi->transfer(PN532_HOSTTOPN532);

    for (uint8_t i = 0; i < headerLen; i++) _spi->transfer(headerBuf[i]);
    for (uint8_t i = 0; i < cmdLen; i++) _spi->transfer(cmd[i]);

    _spi->transfer(dcs);
    _spi->transfer(PN532_POSTAMBLE);

    _spi->endTransaction();
    digitalWrite(_ssPin, HIGH);
  } else if (_interface == PN532_I2C_MODE) {
    _wire->beginTransmission(_i2cAddr);
    _wire->write(PN532_PREAMBLE);
    _wire->write(PN532_STARTCODE1);
    _wire->write(PN532_STARTCODE2);
    _wire->write(frameLen);
    _wire->write(lcs);
    _wire->write(PN532_HOSTTOPN532);

    for (uint8_t i = 0; i < headerLen; i++) _wire->write(headerBuf[i]);
    for (uint8_t i = 0; i < cmdLen; i++) _wire->write(cmd[i]);

    _wire->write(dcs);
    _wire->write(PN532_POSTAMBLE);
    _wire->endTransmission();
  } else if (_interface == PN532_UART_MODE) {
    while (_serial->available()) _serial->read();

    _serial->write(PN532_PREAMBLE);
    _serial->write(PN532_STARTCODE1);
    _serial->write(PN532_STARTCODE2);
    _serial->write(frameLen);
    _serial->write(lcs);
    _serial->write(PN532_HOSTTOPN532);

    if (headerLen > 0) _serial->write(headerBuf, headerLen);
    if (cmdLen > 0) _serial->write(cmd, cmdLen);

    _serial->write(dcs);
    _serial->write(PN532_POSTAMBLE);
  }

  return readAck();
}

bool Bonezegei_PN532::readAck(uint16_t timeout) {
  uint8_t ackBuf[6];
  uint8_t expectedBuf[6] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};

  if (!waitReady(timeout)) return false;

  if (_interface == PN532_SPI_MODE) {
    digitalWrite(_ssPin, LOW);
    _spi->beginTransaction(SPISettings(1000000, LSBFIRST, SPI_MODE0));
    _spi->transfer(0x03);
    for (uint8_t i = 0; i < 6; i++) {
      ackBuf[i] = _spi->transfer(0x00);
    }
    _spi->endTransaction();
    digitalWrite(_ssPin, HIGH);
  } else if (_interface == PN532_I2C_MODE) {
    _wire->requestFrom(_i2cAddr, (uint8_t)7);
    if (_wire->available()) {
      _wire->read();
    }
    for (uint8_t i = 0; i < 6; i++) {
      ackBuf[i] = _wire->read();
    }
  } else if (_interface == PN532_UART_MODE) {
    uint32_t start = millis();
    uint8_t count = 0;
    while (count < 6 && (millis() - start < timeout)) {
      if (_serial->available()) {
        ackBuf[count++] = _serial->read();
      }
    }
    if (count < 6) return false;
  }

  return (memcmp(ackBuf, expectedBuf, 6) == 0);
}

int16_t Bonezegei_PN532::readResponse(uint8_t *buf, uint8_t maxLen, uint16_t timeout) {
  if (!waitReady(timeout)) return -1;

  uint8_t len = 0;

  if (_interface == PN532_SPI_MODE) {
    digitalWrite(_ssPin, LOW);
    _spi->beginTransaction(SPISettings(1000000, LSBFIRST, SPI_MODE0));
    _spi->transfer(0x03);

    if (_spi->transfer(0x00) != 0x00 || _spi->transfer(0x00) != 0x00 || _spi->transfer(0x00) != 0xFF) {
      _spi->endTransaction();
      digitalWrite(_ssPin, HIGH);
      return -2;
    }

    len = _spi->transfer(0x00);
    uint8_t lcs = _spi->transfer(0x00);
    if (((len + lcs) & 0xFF) != 0) {
      _spi->endTransaction();
      digitalWrite(_ssPin, HIGH);
      return -3;
    }

    uint8_t tfi = _spi->transfer(0x00);
    if (tfi != PN532_PN532TOHOST) {
      _spi->endTransaction();
      digitalWrite(_ssPin, HIGH);
      return -4;
    }

    uint8_t payloadLen = len - 1;
    uint8_t dcsSum = tfi;

    for (uint8_t i = 0; i < payloadLen; i++) {
      uint8_t byteVal = _spi->transfer(0x00);
      dcsSum += byteVal;
      if (i < maxLen) buf[i] = byteVal;
    }

    uint8_t dcs = _spi->transfer(0x00);
    _spi->transfer(0x00);

    _spi->endTransaction();
    digitalWrite(_ssPin, HIGH);

    if (((dcsSum + dcs) & 0xFF) != 0) return -5;
    return payloadLen;

  } else if (_interface == PN532_I2C_MODE) {
    _wire->requestFrom(_i2cAddr, (uint8_t)(maxLen + 8));
    if (_wire->available()) _wire->read();

    if (_wire->read() != 0x00 || _wire->read() != 0x00 || _wire->read() != 0xFF) {
      return -2;
    }

    len = _wire->read();
    uint8_t lcs = _wire->read();
    if (((len + lcs) & 0xFF) != 0) return -3;

    uint8_t tfi = _wire->read();
    if (tfi != PN532_PN532TOHOST) return -4;

    uint8_t payloadLen = len - 1;
    uint8_t dcsSum = tfi;

    for (uint8_t i = 0; i < payloadLen; i++) {
      uint8_t byteVal = _wire->read();
      dcsSum += byteVal;
      if (i < maxLen) buf[i] = byteVal;
    }

    uint8_t dcs = _wire->read();
    _wire->read();

    if (((dcsSum + dcs) & 0xFF) != 0) return -5;
    return payloadLen;

  } else if (_interface == PN532_UART_MODE) {
    uint32_t start = millis();
    while (_serial->available() < 5 && (millis() - start < timeout));

    if (_serial->read() != 0x00 || _serial->read() != 0x00 || _serial->read() != 0xFF) {
      return -2;
    }

    len = _serial->read();
    uint8_t lcs = _serial->read();
    if (((len + lcs) & 0xFF) != 0) return -3;

    start = millis();
    while (_serial->available() < (len + 2) && (millis() - start < timeout));

    uint8_t tfi = _serial->read();
    if (tfi != PN532_PN532TOHOST) return -4;

    uint8_t payloadLen = len - 1;
    uint8_t dcsSum = tfi;

    for (uint8_t i = 0; i < payloadLen; i++) {
      uint8_t byteVal = _serial->read();
      dcsSum += byteVal;
      if (i < maxLen) buf[i] = byteVal;
    }

    uint8_t dcs = _serial->read();
    _serial->read();

    if (((dcsSum + dcs) & 0xFF) != 0) return -5;
    return payloadLen;
  }

  return -1;
}