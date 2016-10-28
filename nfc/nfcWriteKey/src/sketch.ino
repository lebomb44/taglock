#include <PN532.h>

#define SCK 13
#define MOSI 11
#define SS 10
#define MISO 12

PN532 nfc(SCK, MISO, MOSI, SS);

//uint8_t default_key[6]= {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
uint8_t lb_keyA[6] = {'s', 'p', 'o', 'c', 'K', 'e'};
uint8_t default_key[6] = {'g', 'e', 'n', 'e', 's', 'i'};
uint8_t lb_keyB[6] = {'g', 'e', 'n', 'e', 's', 'i'};

uint8_t first_name[16] = { 'O', 'l', 'i', 'v', 'i', 'e', 'r', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
uint8_t last_name[16] = { 'C', 'a', 'm', 'b', 'o', 'n', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
uint8_t info_block[16] = { 'L', 'e', 'B', 'o', 'm', 'b', 'H', 'o', 'm', 'e', 'S', 'y', 's', 't', 'e', 'm'};

void setup(void) {
  Serial.begin(9600);
  Serial.println("Welcome to LB NFC Mirafe prog");

  nfc.begin();

  /* First step to check the correct connection of the shield */
  uint32_t firmwareVersion = 0;
  while(0 == firmwareVersion) {
    /* Get the version from the NFC shield */
    firmwareVersion = nfc.getFirmwareVersion();
  }
  /* Print the shield ID */
  Serial.print("Found chip PN5"); Serial.println((firmwareVersion>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((firmwareVersion>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((firmwareVersion>>8) & 0xFF, DEC);
  Serial.print("Supports "); Serial.println(firmwareVersion & 0xFF, HEX);

  /* Configure board to read RFID tags and cards */
  nfc.SAMConfig();

  uint32_t id = 0;
  /* Look for MiFare type cards */
  while(0 == id) {
    /* Get the ID of the Mifare tag */
    id = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A);
    /* If the tag is present we can continue */
    if(0 != id) {
      Serial.print("##### Read card ID= "); Serial.println(id);

      /* The tag has been found. Loop on each trailer */
      for(uint8_t trailer=3; trailer<4; trailer=trailer+4) {
        /* If the tag is new it should have a default key */
        Serial.print("***** Writing Trailer "); Serial.print(trailer, DEC); Serial.println(" *****");
        if(0 != nfc.authenticateBlock(1, id , trailer, KEY_A, default_key)) {
          Serial.println("Authentification successful with default key A");

          /* Read the whole trailer */
          Serial.print("Reading trailer... ");
          uint8_t read_trailer[16] = {0};
          if(0 != nfc.readMemoryBlock(1, trailer, read_trailer)) {
            Serial.println("successful");
            Serial.print("Read trailer:");
            for(uint8_t i=0; i<16; i++) {
              Serial.print(" "); Serial.print(read_trailer[i], HEX);
            }
            Serial.println();
          }
          else {
            Serial.println("FAILED !!!");
            return;
          }

          /* Change the keys in the trailer */
          Serial.print("Generating new trailer...");
          /* Copy the trailer */
          uint8_t write_trailer[16] = {0};
          for(uint8_t i=0; i<16; i++) { write_trailer[i] = read_trailer[i]; }
          for(uint8_t i=0; i<6; i++) {
            /* Insert new key A */
            write_trailer[i] = lb_keyA[i];
            /* Insert new key B */
            write_trailer[10+i] = lb_keyB[i];
          }
          Serial.println(" successful");

          /* Write the new whole trailer */
          Serial.print("Writing trailer... ");
          if(0 != nfc.writeMemoryBlock(1, trailer, write_trailer)) {
            Serial.println("successful");
          }
          else {
            Serial.println("FAILED !!!");
            return;
          }

          /* Read again the whole trailer */
          Serial.print("Reading trailer... ");
          uint8_t new_trailer[16] = {0};
          if(0 != nfc.readMemoryBlock(1, trailer, new_trailer)) {
            Serial.println("successful");
            Serial.print("Read trailer:");
            for(uint8_t i=0; i<16; i++) {
              Serial.print(" "); Serial.print(new_trailer[i], HEX);
            }
            Serial.println();
          }
          else {
            Serial.println("FAILED !!!");
            return;
          }

          /* Compare the new trailer to the one we should have written */
          /* Just patch KEY A area to zero because acces bit does not allow to read KEY A */
          for(uint8_t i=0; i<6; i++) { write_trailer[i] = 0; }
          /* Do the comparison */
          for(uint8_t i=0; i<16; i++) {
            if(new_trailer[i] != write_trailer[i]) {
              Serial.print("Comparison failed on byte "); Serial.print(i, DEC); Serial.println();
              return;
            }
          }
        }
      }

      /* The tag has been modified with new keys. Loop on each block to write the name of the user */
      for(uint8_t block=0; block<4; block++) {
        /* If the tag is new it should have a default key */
        Serial.print("***** Writing Block "); Serial.print(block, DEC); Serial.println(" *****");
        if(0 != nfc.authenticateBlock(1, id , block, KEY_A, lb_keyA)) {
          Serial.println("Authentification successful with lb key A");

          /* Write the new whole block */
          Serial.print("Writing block... ");
          uint8_t * write_block = 0;
          if(0 == (block%4)) { write_block = first_name; }
          else if(1 == (block%4)) { write_block = last_name; }
          else if(2 == (block%4)) { write_block = info_block; }
          else { write_block = 0; }
          if(0 != write_block) {
            if(0 != nfc.writeMemoryBlock(1, block, write_block)) {
              Serial.println("successful");
            }
            else {
              Serial.println("FAILED !!!");
            }
          }

          /* Read again the whole block */
          Serial.print("Reading block... ");
          uint8_t read_block[16] = {0};
          if(0 != nfc.readMemoryBlock(1, block, read_block)) {
            Serial.println("successful");
            Serial.print("Read block:");
            for(uint8_t i=0; i<16; i++) {
              Serial.print(" "); Serial.print(read_block[i], HEX);
            }
            Serial.println();
          }
          else {
            Serial.println("FAILED !!!");
            return;
          }

          /* Compare the read block to the one we should have written */
          /* Do the comparison */
          uint8_t * ref_block = 0;
          if(0 == (block%4)) { ref_block = first_name; }
          else if(1 == (block%4)) { ref_block = last_name; }
          else if(2 == (block%4)) { ref_block = info_block; }
          else { ref_block = 0; }
          if(0 != ref_block) {
            for(uint8_t i=0; i<16; i++) {
              if(read_block[i] != ref_block[i]) {
                Serial.print("Comparison failed on byte "); Serial.print(i, DEC); Serial.println();
                return;
              }
            }
          }
        }
        else {
          Serial.println("FAILED to write block !!!");
          return;
        }
      }

      /* Check that all block are accessible with key B */
      for(uint8_t block=0; block<64; block++) {
        /* If the tag is new it should have a default key */
        Serial.print("***** Checking Block "); Serial.print(block, DEC); Serial.println(" *****");
        if(0 != nfc.authenticateBlock(1, id , block, KEY_B, lb_keyB)) {
          Serial.println("Authentification successful with lb key B");

          /* Read the whole block */
          Serial.print("Reading block... ");
          uint8_t read_block[16] = {0};
          if(0 != nfc.readMemoryBlock(1, block, read_block)) {
            Serial.println("successful");
            Serial.print("Read block:");
            for(uint8_t i=0; i<16; i++) {
              Serial.print(" "); Serial.print(read_block[i], HEX);
            }
            Serial.println();
          }
          else {
            Serial.println("FAILED !!!");
            return;
          }

          /* Compare the read block to the one we should have written */
          /* Do the comparison */
          uint8_t * ref_block = 0;
          if(0 == (block%4)) { ref_block = first_name; }
          else if(1 == (block%4)) { ref_block = last_name; }
          else if(2 == (block%4)) { ref_block = info_block; }
          else { ref_block = 0; }
          if(0 != ref_block) {
            for(uint8_t i=0; i<16; i++) {
              if(read_block[i] != ref_block[i]) {
                Serial.print("Comparison failed on byte "); Serial.print(i, DEC); Serial.println();
                return;
              }
            }
          }
        }
        else {
          Serial.println("FAILED to ccheck block !!!");
          return;
        }
      }
      Serial.print("##### Card with ID= "); Serial.print(id);
      Serial.println(" correctly updated !");
    }
  }
}

void loop(void) {}

