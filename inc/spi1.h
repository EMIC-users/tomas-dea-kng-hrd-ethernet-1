
 // USE
 // How to Include the SPI Driver:
 // 
 // To include and use the SPI driver, use the following syntax:
 // 
 // EMIC:setInput(DEV:_hal/SPI/spi.emic, port={port}, configuracion={configuracion}, pin={pin})           //    
 // Replace the placeholders:
 // - {port}: Specify the desired port for the SPI communication.
 // - {configuracion}: Specify the desired configuration for the SPI.
 // - {pin}: Specify the pin names for the SPI communication.
 // 
 // Note: In the hardware configuration file, pins should be defined as PIN_MISO, PIN_MOSI, PIN_CLK.
 // Use the defined name (PIN) in place of ETH.                                                            //            
 // 
 // Example:
 // 
 // EMIC:setInput(DEV:_hal/SPI/spi.emic, port=1, configuracion=Master, pin=RF)                                //           
 // 
 // This example includes the SPI driver using port 1,
 // configured as a master, and with the SPI communication on the RF pins.

 
#ifndef _PIC_SPI1_MASTER_H_
#define _PIC_SPI1_MASTER_H_

#include <xc.h>

// @brief Initializes the SPI module for communication.
//
// @param mode16    Set to 1 for 16-bit communication, 0 for 8-bit communication.
// @param spi_mode  Selects the SPI mode (0 to 3).
//
// @details This function configures the SPI module for communication based on the
// specified parameters. It sets up pin configurations, data format, clock polarity,
// and clock phase. The SPI module is enabled after initialization.
//
// Example Usage:
// @code
// SPI{port}_init(1, 0); // Initialize SPI with 16-bit communication, Mode 0                                              //
// @endcode
//
// @warning The function assumes that the hardware configuration and pin names are
// correctly defined. Incorrect pin configurations may lead to unexpected behavior.

void SPI1_init (uint8_t mode16, uint8_t spi_mode);

void SPI1_disable();
void SPI1_enable();
uint8_t SPI1_send_async(uint8_t * new_data, uint8_t package_size, uint8_t * proccess_function);


// @brief Exchanges a single byte using SPI communication.
//
// @param data  The byte to be sent over SPI.
//
// @details This function sends a byte over SPI and waits for the transfer to
// complete. It then reads and returns the received byte.
//
// @return      The byte received over SPI.
//
// Example Usage:
// @code
// uint8_t receivedByte = xchangeSPI{port}b_8(0xAA);                                                    //
// @endcode
//
// @warning This function assumes that the SPI module is initialized and configured
// properly before its use. Incorrect configurations may lead to unexpected behavior.
 
uint8_t xchangeSPI1b_8(uint8_t data);


// @brief Exchanges a 16-bit value using SPI communication.
//
// @param data  The 16-bit value to be sent over SPI.
//
// @details This function sends a 16-bit value over SPI and waits for the transfer
// to complete. It then reads and returns the received 16-bit value.
// 
// @return      The 16-bit value received over SPI.
//
// Example Usage:
// @code
// uint16_t receivedValue = xchangeSPI{port}b_16(0x55AA);       //
// @endcode
//
// @warning This function assumes that the SPI module is initialized and configured
// properly before its use. Incorrect configurations may lead to unexpected behavior.

uint16_t xchangeSPI1b_16(uint16_t data);


// @brief Clears the SPI buffer by reading and discarding any remaining data.
//
// @details This function reads and discards any remaining data in the SPI buffer
// to clear it. It is useful for preparing the SPI module for a new set of
// communications.
//
// Example Usage:
// @code
// SPI{port}_clr();                                                                                         //
// @endcode
//
// @warning This function assumes that the SPI module is initialized and configured
// properly before its use. Incorrect configurations may lead to unexpected behavior.

void SPI1_clr(void);

void SPI1_SetSpeed(uint32_t desired_speed_hz);
/** @} doxygen end group definition */
/*==================[end of file]============================================*/
#endif 
