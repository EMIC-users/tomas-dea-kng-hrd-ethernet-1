
 #ifndef __PIC_I2C1_C__
 #define __PIC_I2C1_C__
 
 /*==================[inclusions]=============================================*/
 #include <xc.h>
 #include "inc/I2C1.h"
 #include "inc/system.h"
 
 /*==================[function implementation]================================*/
 
 void Init_Master_I2C1(uint16_t frec,uint8_t address)
 {
    //seteo el baudrate
    I2C1BRG = FCY / ((uint32_t)((uint32_t)frec * 1000) + 1 + ((float)FCY / 10000000));
    I2C1CONbits.SEN = 0;         //Start condition is not in progress
    I2C1CONbits.PEN = 0;
    I2C1CONbits.RSEN = 0;        //Initiates Repeated Start condition on SDAx and SCLx pins. Hardware is clear at the end of master Repeated Start sequence
    I2C1CONbits.RCEN = 0;
    I2C1CONbits.ACKEN = 0;
    I2C1CONbits.ACKDT = 0;       //1 = Sends a NACK during Acknowledge
                                        //0 = Sends an ACK during Acknowledge
    I2C1CONbits.STREN = 0;	    //1 : Enables software or receive clock stretching
                                        //0 : Disables software or receive clock stretching
    I2C1CONbits.GCEN = 1;        // interrupt -> 0 - disabled / 1 - enable
    I2C1CONbits.SMEN = 0;
    I2C1CONbits.DISSLW = 1;      //Slew rate control is disabled
    I2C1CONbits.A10M = 0;
    I2C1CONbits.IPMIEN = 0;
    I2C1CONbits.SCLREL = 1;
    I2C1CONbits.I2CSIDL = 1;     // Discontinues module operation when device enters an Idle mode
    I2C1ADD = address;
    I2C1RCV = 0x0000;
    I2C1TRN = 0x0000;
    I2C1CONbits.I2CEN = 1;       // Enables the I2Cx module and configures the SDAx and SCLx pins as serial port pins
    _MI2C1IE = 1;            // interrupt -> 0 - disabled / 1 - enable
    _SI2C1IE = 1;            // interrupt -> 0 - disabled / 1 - enable
 
 }
 
 void CollisionReset_I2C1()
 {
    I2C1STATbits.BCL = 0;
 }
 
 void OverflowReset_I2C1()
 {
    I2C1STATbits.I2COV = 0;
 }
 
 uint8_t IsStartI2c1()
 {
    return I2C1STATbits.S;
 }
 
 uint8_t IsCollisionDetectI2c1()
 {
    return I2C1STATbits.BCL;
 }
 
 uint8_t IsStopI2c1()
 {
    return I2C1STATbits.P;
 }
 
 uint8_t IsReceiveBufferFullI2c1()
 {
    return I2C1STATbits.RBF;
 }
 
 uint8_t IsDataOrAddressI2c1()
 {
    return I2C1STATbits.D_A;
 }
 
 uint8_t IsReceiveOverflowI2c1()
 {
    return I2C1STATbits.I2COV;
 }
 
 void Start_I2C1()
 {
    //This function generates an I2C start condition and returns status
    //of the Start.
    I2C1CONbits.SEN = 1;     //Generate Start Condition
    while (I2C1CONbits.SEN);
 
 }
 
 void Stop_I2C1()
 {
    //This function generates an I2C stop condition and returns status
    //of the Stop.
    I2C1CONbits.PEN = 1;     //Generate Stop Condition
    while (I2C1CONbits.PEN);
 }
 void Write_I2C1(unsigned char byte)
 {
    //This function transmits the byte passed to the function
    I2C1TRN = byte;					//Load byte to I2C1 Transmit buffer
    if (!1)
    {
       while (I2C1STATbits.TRSTAT);	//Wait for bus to be idle
    }
 }
 
 uint8_t Read_I2C1(uint8_t Ack)
 {
    uint8_t data = 0;

    //TODO: Revisar la viabilidad de implementar la macro interrupt  en la api del I2C
    
    if (!1)
    {
       I2C1CONbits.RCEN = 1;			//Enable Master receive
       Nop();
       while (!I2C1STATbits.RBF);		//Wait for receive bufer to be full
       data = I2C1RCV;
       if(Ack)
           I2C1CONbits.ACKDT = 1;
       else
           I2C1CONbits.ACKDT = 0;
       I2C1CONbits.ACKEN = 1;
       while (I2C1CONbits.ACKEN);
    }
    else
    {
        data = I2C1RCV;
        I2C1CONbits.SCLREL = 1;
    }
    return(data);				//Return data in buffer
 }
 
 void __attribute__((interrupt(auto_psv))) _SI2C1Interrupt( void )
 {
    _SI2C1IF = 0;
    I2c_driver_callback_slave();
 }
 
 void __attribute__((interrupt(auto_psv))) _MI2C1Interrupt( void )
 {
    _MI2C1IF = 0;
    I2c_driver_callback_master();
 }
 
 /*==================[end of file]============================================*/
 #endif
