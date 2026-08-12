
/*==================[inclusions]=============================================*/
#include <xc.h>
#include "inc/gpio.h"
#include "inc/spi1.h"
#include "inc/system.h"
/*==================[data declaration]==============================*/
uint8_t * spi1_pending_data;
uint8_t spi1_response_data[32];

uint8_t spi1_package_size = 8;
uint8_t spi1_ready = 1;
int spi1_indice = 0;
/******************************************************************************/

void (*spi1_fun_ptr)(uint8_t *, uint8_t);

void SPI1_init (uint8_t mode16, uint8_t spi_mode)
{
    HAL_GPIO_PinCfg(ETH_MOSI, GPIO_OUTPUT);
	HAL_GPIO_PinCfg(ETH_CLK, GPIO_OUTPUT);
    HAL_GPIO_PinCfg(ETH_MISO, GPIO_INPUT);

	__builtin_write_OSCCONL(OSCCON & 0xbf); // unlock PPS

	RPOUT_ETH_MOSI	= _RPOUT_SDO1;
	RPOUT_ETH_CLK	= _RPOUT_SCK1OUT;
	_SDI1R = RPIN_ETH_MISO;

	__builtin_write_OSCCONL(OSCCON | 0x40); // lock PPS

	SPI1STATbits.SPIEN   = 0;             //Disable SPI port
	SPI1STATbits.SPISIDL = 0;             //Continue module operation in Idle mode
    SPI1BUF              = 0;             //Clear SPI buffer data Master
    _SPI1IF      = 0;             //Clear flag interrupt
    _SPI1IE      = 0;             //Disable Interrupt
    _SPI1IP      = 1;             //Set the interrupt priority to low
    SPI1CON1bits.DISSCK  = 0;             //Internal SPI Clock is Enabled
    SPI1CON1bits.DISSDO  = 0;             //SDOx pin is controlled by the module (Enable SDO)
    SPI1CON1bits.MODE16  = mode16;        //Communication is word-wide (8 bits or 16 bits)
    switch (spi_mode)
    {
    case 0:
    	SPI1CON1bits.SMP     = 0;             //Input data is sampled at middle of data output time
    	SPI1CON1bits.CKP     = 0;             //Idle state for clock is a low level; active state is a high level
    	SPI1CON1bits.CKE     = 1;             //Serial output data changes on transition from active clock state to Idle clock state  
    	break;
    case 1:
    	SPI1CON1bits.SMP     = 0;             //Input data is sampled at middle of data output time
    	SPI1CON1bits.CKP     = 1;             //Idle state for clock is a high level; active state is a low level
    	SPI1CON1bits.CKE     = 0;             //Serial output data changes on transition from Idle clock state to active clock state
    	break;
    case 2:
    	SPI1CON1bits.SMP     = 1;             //Input Data is sampled at the end of data output time
    	SPI1CON1bits.CKP     = 0;             // Idle state for clock is a low level; active state is a high level
    	SPI1CON1bits.CKE     = 1;             //Serial output data changes on transition from active clock state to Idle clock state  
    	break;
    case 3:
    	SPI1CON1bits.SMP     = 1;             //Input Data is sampled at the end of data output time
    	SPI1CON1bits.CKP     = 1;             //Idle state for clock is a high level; active state is a low level
    	SPI1CON1bits.CKE     = 0;             //Serial output data changes on transition from Idle clock state to active clock state
    	break;
    case 4: 
        SPI1CON1bits.SMP     = 1;             //Input Data is sampled at the end of data output time
    	SPI1CON1bits.CKP     = 0;             //Idle state for clock is a low level; active state is a high level
    	SPI1CON1bits.CKE     = 0;             //Serial output data changes on transition from Idle clock state to active clock state  
        break;
    case 5: 
        SPI1CON1bits.SMP     = 0;             //Input data is sampled at middle of data output time
    	SPI1CON1bits.CKP     = 1;             //Idle state for clock is a high level; active state is a low level
    	SPI1CON1bits.CKE     = 1;             //Serial output data changes on transition from active clock state to Idle clock state  
        break;
    case 6:
        SPI1CON1bits.SMP     = 0;             //Input data is sampled at middle of data output time
    	SPI1CON1bits.CKP     = 0;             //Idle state for clock is a low level; active state is a high level
    	SPI1CON1bits.CKE     = 0;             //Serial output data changes on transition from Idle clock state to active clock state
        break;
    case 7:
        SPI1CON1bits.SMP     = 1;             //Input Data is sampled at the end of data output time
    	SPI1CON1bits.CKP     = 1;             //Idle state for clock is a high level; active state is a low level
    	SPI1CON1bits.CKE     = 1;             //Serial output data changes on transition from active clock state to Idle clock state 
        break;
    }
    SPI1CON1bits.SSEN    = 0;
    SPI1CON1bits.SPRE    = 6;         // secondary prescale as  SPRE=7 1:1 -- SPRE=6 2:1 ... SPRE=0 8:1
    SPI1CON1bits.PPRE    = 3; 		  // Primary prescale as   PPRE=3 1:1 -- PPRE=2 4:1 -- PPRE=1 16:1 -- PPRE=0 64:1
    SPI1CON1bits.MSTEN   = 1;         //Master Mode Enabled
    SPI1CON2             = 0;         //non-framed mode

    SPI1STATbits.SPIEN   = 1;         //Enable SPI Module

}

void SPI1_enable(){
    SPI1STATbits.SPIEN   = 1;             //Enable SPI port
}

void SPI1_disable(){
    SPI1STATbits.SPIEN   = 0;             //Disable SPI port
}

#define spi1_put(data) SPI1BUF=data
#define spi1_get() SPI1BUF

uint8_t xchangeSPI1b_8(uint8_t data)
{
    SPI1BUF = data;					    // write to buffer for TX
    while(!SPI1STATbits.SPIRBF);        // wait for transfer to complete
    uint8_t ret = SPI1BUF;
    return ret;    				        // read the received value
}

uint16_t xchangeSPI1b_16(uint16_t data)
{
    SPI1BUF = data;					    // write to buffer for TX
    while(!SPI1STATbits.SPIRBF);        // wait for transfer to complete
    uint16_t ret = SPI1BUF;
    return ret;    				        // read the received value
}


void SPI1_clr()
{
    uint8_t damy;
    while(SPI1STATbits.SPIRBF)
    {
        damy = SPI1BUF;
    }

}


uint8_t SPI1_send_async(uint8_t * new_data, uint8_t package_size, uint8_t * proccess_function)
{
    if (spi1_ready)
    {
        _SPI1IE      = 1;             //Enable Interrupt
        spi1_fun_ptr = (void*)proccess_function;              //revisar cambios realizados --Luciano 19/02
        spi1_package_size = package_size;
        spi1_pending_data = new_data;
        spi1_indice = 0;
        spi1_ready = 0;
        SPI1BUF = spi1_pending_data[spi1_indice];
        spi1_indice++;
        return 1;
    }
    return 0;
}

void __attribute__((interrupt(auto_psv))) _SPI1Interrupt (void)
{
    if (!spi1_ready)
    {
        spi1_response_data[spi1_indice - 1] = SPI1BUF;
        if (spi1_indice < spi1_package_size)
        {
            SPI1BUF = spi1_pending_data[spi1_indice];
            spi1_indice++;
            return;
        }
        else
        {
            if (spi1_fun_ptr)
            {
                (*spi1_fun_ptr)(spi1_response_data, spi1_package_size);
            }
            spi1_ready = 1;
            _SPI1IE      = 0;             //Disable Interrupt
        }
    }  
}

void SPI1_SetSpeed(uint32_t desired_speed_hz)
{
    SPI1_disable();
    uint8_t best_ppre = 0;
    uint8_t best_spre = 0;
    uint32_t best_error = 0xFFFFFFFF;

    // Recorrer todas las combinaciones posibles de PPRE y SPRE
    for (uint8_t ppre = 0; ppre < 4; ppre++) {
        uint8_t ppre_value = (ppre == 3) ? 1 : (4 << (2 - ppre));
        for (uint8_t spre = 0; spre < 8; spre++) {
            uint8_t spre_value = 8 - spre;
            if (ppre == 3 && spre == 7) {
                // Evitar el caso donde ambos son 1:1
                continue;
            }

            uint32_t spi_speed = FCY / (ppre_value * spre_value);
            uint32_t error = (spi_speed > desired_speed_hz) ? spi_speed - desired_speed_hz : desired_speed_hz - spi_speed;

            // Seleccionar la combinación con el error más bajo
            if (error < best_error) {
                best_error = error;
                best_ppre = ppre;
                best_spre = spre;
            }
        }
    }

    // Configurar el registro SPIxCON1 con los mejores valores encontrados
    SPI1CON1bits.PPRE = best_ppre;
    SPI1CON1bits.SPRE = best_spre;
    SPI1_enable();
}
/** @} doxygen end group definition */
/*==================[end of file]============================================*/
