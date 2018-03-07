#include "SPIFlash.h"
#include <SPI.h>
 
#define SERIAL_BAUD      115200
char input = 0;
long lastPeriod = -1;

#ifdef __AVR_ATmega1284P__
#define LED           15 // Moteino MEGAs have LEDs on D15
#define FLASH_SS      23 // and FLASH SS on D23
#else
#define LED           9 // Moteinos have LEDs on D9
#define FLASH_SS      8 // and FLASH SS on D8
#endif

#define MAX_NAME_SIZE 10
#define MAX_EXTENSION_SIZE 4
#define MAJOR_VER 0
#define MINOR_VER 1
typedef struct flashFile
{
  uint8_t permissions; //read/write/append/deleted?
  uint16_t firstDataBlock;
  char name[MAX_NAME_SIZE];
  char extension[MAX_EXTENSION_SIZE];
} flashFile;


typedef struct flashFileSystem
{
  uint32_t flashSize;
  uint32_t blockSize;
  uint16_t majorVersion;
  uint16_t minorVersion;
} flashFileSystem;

#define BLOCK_FULL 0xDD
#define BLOCK_HEADER 0x43

typedef struct blockHeader
{
  uint8_t header = BLOCK_HEADER;
  uint16_t nextBlock;
  uint8_t full;
} blockHeader;


SPIFlash flash(FLASH_SS, 0xEF30);
//Flash filesystem id, if found at address 0 of a memory we know we have a valid filesystem
const uint32_t FLASHFILESYSID = 0xAA55AA55;
uint32_t nextFileSlot = 0;//TODO: write function to scan for this
#define BLOCK_SIZE 4096



void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.print("Start...");

  if (flash.initialize())
  {
    Serial.println("Init OK!");
    Blink(LED, 20, 10);
  }
  else
    Serial.println("Init FAIL!");

  createFlashFileSystem(512000UL, 4096UL);
  
    boolean v = findFirstFileSlot(&nextFileSlot);
    Serial.println(nextFileSlot);
    Serial.println(v);


    
    //fileExists("test", "txt");*/
  delay(1000);
}

void loop() {
  // Handle serial input (to allow basic DEBUGGING of FLASH chip)
  // ie: display first 256 bytes in FLASH, erase chip, write bytes at first 10 positions, etc
  if (Serial.available() > 0) {
    input = Serial.read();
    if (input == 'd') //d=dump flash area
    {
      Serial.println("Flash content:");
      int counter = 0;

      while (counter <= 256) {
        Serial.print(flash.readByte(counter++), HEX);
        Serial.print('.');
      }

      Serial.println();
    }
    else if (input == 'e')
    {
      Serial.print("Erasing Flash chip ... ");
      flash.chipErase();
      while (flash.busy());
      Serial.println("DONE");
    }
    else if (input == 'i')
    {
      Serial.print("DeviceID: ");
      Serial.println(flash.readDeviceId(), HEX);
    }
    else if (input == 'l')
    {
      listFiles();
    }
    else if (input == 'c')
    {
      createFile("abcdefg", "zzz");
    }
  }

  // Periodically blink the onboard LED while listening for serial commands
  if ((int)(millis() / 500) > lastPeriod)
  {
    lastPeriod++;
    pinMode(LED, OUTPUT);
    digitalWrite(LED, lastPeriod % 2);
  }
}

void Blink(byte PIN, int DELAY_MS, byte loops)
{
  pinMode(PIN, OUTPUT);
  while (loops--)
  {
    digitalWrite(PIN, HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN, LOW);
    delay(DELAY_MS);
  }
}
