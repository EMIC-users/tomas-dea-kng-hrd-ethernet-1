#include <xc.h>
#include <libpic30.h>

// Puerto I2C del bus EMIC (convención EMIC system.i2c; usado por app y bootloader)

#define TRIS_Led1	                _TRISB7
#define PORT_Led1 	                _RB7
#define PIN_Led1 	                _RB7
#define LAT_Led1 	                _LATB7
#define ODC_Led1	                _ODB7
#define RPOUT_Led1		            RPOR3bits.RP7R
#define RPIN_Led1		            7
#define CN_Led1		                23
// Bus I2C EMIC (I2C1 alterno por fuse I2C1SEL=SEC): SCL=B6, SDA=B5
#define TRIS_SCL	                _TRISB6
#define PIN_SCL 	                _RB6
#define LAT_SCL 	                _LATB6
#define ODC_SCL	                _ODB6
#define RPOUT_SCL		            RPOR3bits.RP6R
#define RPIN_SCL		            6
#define CN_SCL		                24
#define TRIS_SDA	                _TRISB5
#define PIN_SDA 	                _RB5
#define LAT_SDA 	                _LATB5
#define ODC_SDA	                _ODB5
#define RPOUT_SDA		            RPOR2bits.RP5R
#define RPIN_SDA		            5
#define CN_SDA		                27
#define TRIS_ETH_RST	                _TRISB10
#define PORT_ETH_RST 	                _RB10
#define LAT_ETH_RST 	                _LATB10
#define ODC_ETH_RST	                _ODB10
#define PIN_ETH_RST 	                _RB10
#define RPOUT_ETH_RST	                RPOR5bits.RP10R
#define RPIN_ETH_RST	                10
#define CN_ETH_RST		                16
#define TRIS_ETH_CS	                _TRISB11
#define PORT_ETH_CS 	                _RB11
#define LAT_ETH_CS 	                _LATB11
#define ODC_ETH_CS	                _ODB11
#define PIN_ETH_CS 	                _RB11
#define RPOUT_ETH_CS		            RPOR5bits.RP11R
#define RPIN_ETH_CS		            11
#define CN_ETH_CS		                15
#define TRIS_ETH_CLK	                _TRISB12
#define PORT_ETH_CLK 	                _RB12
#define LAT_ETH_CLK 	                _LATB12
#define ODC_ETH_CLK	                _ODB12
#define PIN_ETH_CLK 	                _RB12
#define RPOUT_ETH_CLK		            RPOR6bits.RP12R
#define RPIN_ETH_CLK		            12
#define CN_ETH_CLK		                14
#define ADC_value_ETH_CLK              Buffer_entradas[12]
#define HAL_SetAnalog_ETH_CLK()        {_PCFG12=0;\
                                        adc_addAnalogChannel(12);}

#define TRIS_ETH_MOSI	                _TRISB13
#define PORT_ETH_MOSI 	                _RB13
#define LAT_ETH_MOSI 	                _LATB13
#define ODC_ETH_MOSI	                _ODB13
#define PIN_ETH_MOSI 	                _RB13
#define RPOUT_ETH_MOSI		            RPOR6bits.RP13R
#define RPIN_ETH_MOSI		            13
#define CN_ETH_MOSI		                13
#define ADC_value_ETH_MOSI              Buffer_entradas[11] 
#define HAL_SetAnalog_ETH_MOSI()        {_PCFG11=0;\
                                        adc_addAnalogChannel(11);}

#define TRIS_ETH_MISO	                _TRISB14
#define PORT_ETH_MISO 	                _RB14
#define LAT_ETH_MISO 	                _LATB14
#define ODC_ETH_MISO	                _ODB14
#define PIN_ETH_MISO 	                _RB14
#define RPOUT_ETH_MISO	                RPOR7bits.RP14R
#define RPIN_ETH_MISO	                14
#define CN_ETH_MISO		                12
#define ADC_value_ETH_MISO              Buffer_entradas[10] 
#define HAL_SetAnalog_ETH_MISO()        {_PCFG10=0;\
                                        adc_addAnalogChannel(10);}

#define TRIS_ETH_INT	                _TRISB15
#define PORT_ETH_INT 	                _RB15
#define LAT_ETH_INT 	                _LATB15
#define ODC_ETH_INT	                _ODB15
#define PIN_ETH_INT 	                _RB15
#define RPOUT_ETH_INT	                RPOR7bits.RP15R
#define RPIN_ETH_INT	                15
#define CN_ETH_INT		                11
#define ADC_value_ETH_INT              Buffer_entradas[9] 
#define HAL_SetAnalog_ETH_INT()        {_PCFG9=0;\
                                        adc_addAnalogChannel(9);}
