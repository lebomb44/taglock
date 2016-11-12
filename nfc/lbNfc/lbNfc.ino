/**************************************************************************/
/*! 
    This example will wait for any ISO14443A card or tag, and
    depending on the size of the UID will attempt to read from it.
   
    If the card has a 4-byte UID it is probably a Mifare
    Classic card, and the following steps are taken:
   
    - Authenticate block 4 (the first block of Sector 1) using
      the default KEYA of 0XFF 0XFF 0XFF 0XFF 0XFF 0XFF
    - If authentication succeeds, we can then read any of the
      4 blocks in that sector (though only block 4 is read here)
	 
    If the card has a 7-byte UID it is probably a Mifare
    Ultralight card, and the 4 byte pages can be read directly.
    Page 4 is read by default since this is the first 'general-
    purpose' page on the tags.

    To enable debug message, define DEBUG in PN532/PN532_debug.h
*/
/**************************************************************************/

#include <Cmd.h>
#include <SPI.h>
#include <PN532_SPI.h>
#include "PN532.h"

PN532_SPI pn532spi(SPI, 10);
PN532 nfc(pn532spi);

void setup(void) {
  Serial.begin(115200);
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();

  cmdInit();
  cmdAdd("nfcReadBlock", "Read target block", nfcReadBlock);
  cmdAdd("nfcReadTarget", "Read Target", nfcReadTarget);
  cmdAdd("nfcInit", "NFC Init", nfcInit);
  cmdAdd("help", "Help", help);
  cmdStart();
}

void help(int arg_cnt, char **args) {
  cmdList();
}

void loop(void) {
  cmdPoll();
}

uint8_t nfcSuccessCmd = 0;
uint8_t nfcUID[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
uint8_t nfcUIDLength = 0;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

void nfcInit(int arg_cnt, char **args) {
  nfcSuccessCmd = 0;
  nfcUIDLength = 0;
}

void nfcReadTarget(int arg_cnt, char **args) {
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  nfcSuccessCmd = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nfcUID, &nfcUIDLength);
  
  if (nfcSuccessCmd) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(nfcUIDLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(nfcUID, nfcUIDLength);
    Serial.println("");
  }
  else {
    Serial.println("No NFC tag found");
  }
}

void nfcReadBlock(int arg_cnt, char **args) {
  uint8_t block_begin = 0;
  uint8_t block_end = 0;

  if(1 == arg_cnt) {
    block_begin = 0;
    block_end = 64;
  }
  else if(2 == arg_cnt) {
    block_begin = cmdStr2Num(args[1], 10);
    block_end = block_begin;
  }
  else {
    Serial.println("Bad arguments");
    return;
  }

  if (nfcSuccessCmd) {
    if (nfcUIDLength == 4) {
      // We probably have a Mifare Classic card ... 
      Serial.println("Seems to be a Mifare Classic card (4 byte UID)");
	  
      // Now we need to try to authenticate it for read/write access
      // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
      Serial.println("Trying to authenticate block 4 with default KEYA value");
      uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	  
	  // Start with block 4 (the first block of sector 1) since sector 0
	  // contains the manufacturer data and it's probably better just
	  // to leave it alone unless you know what you're doing
    for(uint8_t blockn=block_begin; blockn<block_end; blockn++) {
      nfcSuccessCmd = nfc.mifareclassic_AuthenticateBlock(nfcUID, nfcUIDLength, 4, 0, keya);
      if (nfcSuccessCmd)
      {
        Serial.print("Block "); Serial.print(blockn); Serial.println(" unlocked");
        uint8_t data[16];
		
        // If you want to write something to block 4 to test with, uncomment
		// the following line and this text should be read back in a minute
        // data = { 'a', 'd', 'a', 'f', 'r', 'u', 'i', 't', '.', 'c', 'o', 'm', 0, 0, 0, 0};
        // success = nfc.mifareclassic_WriteDataBlock (4, data);

        // Try to read the contents of block 4
        nfcSuccessCmd = nfc.mifareclassic_ReadDataBlock(4, data);
		
        if (nfcSuccessCmd)
        {
          // Data seems to have been read ... spit it out
          Serial.println("Reading Block 4:");
          nfc.PrintHexChar(data, 16);
          Serial.println("");
		  
          // Wait a bit before reading the card again
          delay(1000);
        }
        else
        {
          Serial.println("Ooops ... unable to read the requested block.  Try another key?");
        }
      }
      else
      {
        Serial.println("Ooops ... authentication failed: Try another key?");
      }
    }
    }
  }
}

