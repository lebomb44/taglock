#include <SPI.h>
#include <PN532_SPI.h>
#include "PN532.h"

PN532_SPI pn532spi(SPI, 10);
PN532 nfc(pn532spi);

uint8_t keyNb = 0; /* KeyA = 0, KeyB = 1 */
uint8_t keyValue[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
//uint8_t keyValue[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//uint8_t keyValue[6] = { 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };
//uint8_t keyValue[6] = { 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0 };
//uint8_t keyValue[6] = { 0xa1, 0xb1, 0xc1, 0xd1, 0xe1, 0xf1 };
//uint8_t keyValue[6] = { 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5 };
//uint8_t keyValue[6] = { 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5 };
//uint8_t keyValue[6] = { 0x4d, 0x3a, 0x99, 0xc3, 0x51, 0xdd };
//uint8_t keyValue[6] = { 0x1a, 0x98, 0x2c, 0x7e, 0x45, 0x9a };
//uint8_t keyValue[6] = { 0xd3, 0xf7, 0xd3, 0xf7, 0xd3, 0xf7 };
//uint8_t keyValue[6] = { 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };

void setup(void) {
  Serial.begin(115200);
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); /* halt */
  }
  /* Got ok data, print it out ! */
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  /* Configure board to read RFID tags */
  nfc.SAMConfig();

  Serial.println("Waiting for an ISO14443A Card ...");
}


void loop(void) {
  uint8_t success = 0;
  uint8_t uid[7] = { 0, 0, 0, 0, 0, 0, 0 };  /* Buffer to store the returned UID */
  uint8_t uidLength = 0;                     /* Length of the UID (4 or 7 bytes depending on ISO14443A card type) */

  /* Wait for an ISO14443A type cards (Mifare, etc.).  When one is found */
  /* 'uid' will be populated with the UID, and uidLength will indicate */
  /* if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight) */
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  if (success) {
    /* Display some basic information about the card */
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");

    if (4 == uidLength) {
      /* We probably have a Mifare Classic card ... */
      Serial.println("Seems to be a Mifare Classic card (4 byte UID)");
    
      /* Now we need to try to authenticate it for read/write access */
      Serial.print("Trying to authenticate blocks with KEY: ");
      if(0 == keyNb) { Serial.println("A"); }
      else if(1 == keyNb) { Serial.println("B"); }
      else { Serial.println("UNKNOWN"); }
      Serial.print("Key value: "); nfc.PrintHex(keyValue, 6);

      /* Read all the blocks */
      for(uint8_t blockNb=0; blockNb<64; blockNb++) {
        success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, blockNb, keyNb, keyValue);
        if (success) {
          uint8_t data[16];
          /* Try to read the contents of block blockNb */
          success = nfc.mifareclassic_ReadDataBlock(blockNb, data);
          if (success) {
            /* Data seems to have been read ... spit it out */
            Serial.print("Block "); Serial.print(blockNb); Serial.print(":");
            nfc.PrintHexChar(data, 16);
          }
        }
      }
    }
  }
  delay(3000);
}

