typedef struct fileSysInfo
{
  uint8_t valid = 0x00;

} fileSysInfo;

fileSysInfo info; //TODO: better name pls

boolean createFlashFileSystem(uint32_t flashSize, uint32_t blockSize)
{

  uint32_t id = 0;
  readStruct(0, &id, sizeof(FLASHFILESYSID));
  Serial.println(id, HEX);

  if (id == FLASHFILESYSID)
  {
    flashFileSystem fFS;
    readStruct(sizeof(FLASHFILESYSID), (void *)&fFS, sizeof(fFS));
    Serial.print("Flash size:");
    Serial.println(fFS.flashSize);
    Serial.print("Block size:");
    Serial.println(fFS.blockSize);
    Serial.print("Version:");
    Serial.print(fFS.majorVersion);
    Serial.print(".");
    Serial.println(fFS.minorVersion);

    return false;
  }

  Serial.println("Erasing Chip");
  flash.chipErase();
  while (flash.busy());


  flashFileSystem fFS;
  fFS.flashSize = flashSize;
  fFS.blockSize = blockSize;
  fFS.majorVersion = MAJOR_VER;
  fFS.minorVersion = MINOR_VER;
  Serial.println("Writing Filesys info");

  writeStruct(sizeof(FLASHFILESYSID), (void *)&fFS, sizeof(flashFileSystem));
  writeStruct(0, (void *)&FLASHFILESYSID, sizeof(FLASHFILESYSID));

  return true;


}
//write header last always in case of power cut
//copy data to clear page, erase, then back

//TODO: some kind of cli
#define DIR_START (sizeof(FLASHFILESYSID)+sizeof(flashFileSystem))
void listFiles()
{

  flashFile fFile;
  uint32_t address = DIR_START;
  readStruct(address, &fFile, sizeof(flashFile));
  while (fFile.permissions != 0xFF)
  {
    printFileInfo(&fFile);
    address += sizeof(flashFile);
    readStruct(address, &fFile, sizeof(flashFile));
  }

}


static void printFileInfo(flashFile * fFile)
{
  char buf[128];

  sprintf(buf, "%x %s.%s -> %x\n", fFile->permissions, fFile->name, fFile->extension, fFile->firstDataBlock);
  Serial.print(buf);
}

/*
  boolean fileExists(const char * name, const char * extension)
  {
  size_t fileStructSize = sizeof(flashFile);

  flashFile ff;

  uint32_t address = 0;

  do {
    readStruct(address, (void *)&ff, sizeof(flashFile));

    if ( !(strncmp(ff.name, name, MAX_NAME_SIZE)) &&
         !(strncmp(ff.extension, extension, MAX_EXTENSION_SIZE)) )
    {
      return true;
    }

    address += sizeof(flashFile);

  } while (address != nextFileSlot);

  return false;
  }
*/
static boolean returnFreeBlock(uint16_t * freeBlockPointer)
{
/*
  if ( !(info.valid) )
  {
    
    return false;
  }*/


  //TODO: get this form a fielsys struct
  const uint16_t totalNumBlocks = 512000UL / 4096UL;


  //Iterate through blocks looking for one with a blank header
  for (uint16_t blockCounter = 1; blockCounter < totalNumBlocks; blockCounter++)
  {
    uint8_t header = flash.readByte(blockCounter * 4096UL);

    if (header == 0xFF)
    {
      flash.writeByte(blockCounter * 4096UL, BLOCK_HEADER);
      *freeBlockPointer = blockCounter;
      return true;
    }
  }
  return false;
}

//remove parameter and put the pointer into the filesysinfo struct
boolean findFirstFileSlot(uint32_t * nextFileSlotPointer)
{
  flashFile ff;

  int32_t remaining = BLOCK_SIZE;
  uint32_t address = DIR_START;

  do
  {
    readStruct(address, (void *)&ff, sizeof(flashFile));

    address += sizeof(flashFile);


    if ( (BLOCK_SIZE - sizeof(flashFile)) < 0)
    {
      return false;
    }
  } while ( ff.permissions != 0xFF );

  *nextFileSlotPointer = address - sizeof(flashFile);
  return true;
}

//TODO: ZERO out char arrays before writing
boolean createFile(const char * name, const char * extension)
{



  //TODO:we currently make no attempt to keep names unique
  flashFile ff;
  ff.permissions = 0xAA;
  strlcpy((char *)&ff.name, name, MAX_NAME_SIZE);
  strlcpy((char *)&ff.extension, extension, MAX_EXTENSION_SIZE);


  //TODO: check file doesnt exist
  /*f ( !fileExists(&ff) )
  {
    return false;
  }*/

//TODO: Chain all the free blocks together?
  
  //TODO: find a free block and fail out if none available
  //TODO: should check that there is space for the filename etc first
  if ( !returnFreeBlock(&ff.firstDataBlock) )
  {
    return false;
  }

  Serial.println(ff.firstDataBlock);
  flashFile fr;
  readStruct(nextFileSlot, (void *)&fr, sizeof(flashFile));

  if (fr.permissions != 0xFF)
  {
    Serial.print("Slot not empty: ");
    Serial.println(ff.permissions, HEX);

    //File already here
    return false;
  }
  writeStruct(nextFileSlot, (void *)&ff, sizeof(flashFile));
  nextFileSlot += sizeof(flashFile);
  return true;
}

void readStruct(uint32_t address, void * structPointer, uint32_t size)
{

  uint8_t * p = (uint8_t*)structPointer;
  for (uint32_t i = address; i < (address + size); i++)
  {
    *p++ = flash.readByte(i);
  }
}


void writeStruct(uint32_t address, const void * structPointer, uint32_t size)
{
  //TODO: this loop can be tidied by pushing structPointer into the for loop
  for (uint32_t i = address; i < (address + size); i++)
  {
    flash.writeByte(i, *((uint8_t *)structPointer++));
  }
}
