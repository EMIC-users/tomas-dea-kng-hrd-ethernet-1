
#ifndef __PIC_I2C1_H__
#define __PIC_I2C1_H__

/*==================[inclusions]=============================================*/

#include <xc.h>

/*==================[macros]=================================================*/


/*==================[typedef]================================================*/


/*==================[internal function declaration]==========================*/

void Init_Master_I2C1(uint16_t frec,uint8_t address);

void CollisionReset_I2C1();

void OverflowReset_I2C1();
void Start_I2C1();
void Stop_I2C1();
void Write_I2C1(unsigned char byte);

uint8_t Read_I2C1(uint8_t Ack);

uint8_t IsStartI2c1();

uint8_t IsCollisionDetectI2c1();

uint8_t IsStopI2c1();

uint8_t IsReceiveBufferFullI2c1();

uint8_t IsDataOrAddressI2c1();

uint8_t IsReceiveOverflowI2c1();

extern void I2c_driver_callback_slave(void);
extern void I2c_driver_callback_master(void);

/*==================[end of file]============================================*/
#endif

