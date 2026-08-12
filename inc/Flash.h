#ifndef __FLASH_H__
#define __FLASH_H__

#include <xc.h>

#include <libpic30.h> //Contains _FLASH_PAGE

#define NVMOP_PAGE_ERASE _FLASH_ERASE_CODE          //Code for page erase, from libpic30
#define NVMOP_WORD_PROGRAM _FLASH_WRITE_WORD_CODE   //Code for word write, from libpic30

extern const uint16_t __attribute__((space(prog),address(0x6000),section("myFlashSpace"))) myFlashMemory[_FLASH_PAGE];
extern uint16_t offsetPutc3B;
extern char bufferPutc3B[4];
extern uint16_t offset;

void flushProgramPutc3B(void);
void putc_program3B(char d);
void ReadMem(uint16_t AddressUpper,uint16_t Address, char* buff);
void WriteMem(uint16_t AddressUpper,uint16_t Address, char* buff);
void ErasePage(uint16_t AddressUpper,uint16_t Address);
char getc_program3B(void);



#endif //__SYSTEM_H__
