
#ifndef __DRIVER_I2C1_C__
#define __DRIVER_I2C1_C__

/*==================[inclusions]=============================================*/
#include <xc.h>
#include "inc/I2C1.h"
#include "inc/I2C1_driver.h"
#include "inc/stream.h"
#include "inc/streamOut.h"
#include "inc/streamIn.h"
#include "inc/myId.h"

/*==================[internal data declaration]==============================*/
#ifndef i2c_frame_indentifier
#define i2c_frame_indentifier 0
#endif

#ifndef MAX_I2C_IN
#define MAX_I2C_IN     512
#endif
#ifndef MAX_I2C_OUT
#define MAX_I2C_OUT    512
#endif

#define     FrameQty        16

#ifdef _I2C_ID
uint8_t I2C_ID = _I2C_ID;
#else
#error _I2C_ID no definido
#endif

static unsigned char i2cInDataBuffer[MAX_I2C_IN];
static unsigned char i2cOutDataBuffer[MAX_I2C_OUT];

static uint16_t i2cInFrameBuffer[FrameQty];
static uint16_t i2cOutFrameBuffer[FrameQty];

stream_t i2c1InStream = 
{
	.data_idx_entr = 0,
	.data_idx_sal = 0,
	.data_idx_sal_aux = 0,
	.data_count_entr = 0,
	.data_count_sal = 0,
	.data_count_sal_aux = 0,
	.frame_idx_entr = 0,
	.frame_idx_sal = 0,
	.frame_count = 0,
	.frame_fifo 	= i2cInFrameBuffer,
	.data_fifo 	= i2cInDataBuffer,
	.frame_indentifier = i2c_frame_indentifier,
	.data_idx_mask = MAX_I2C_IN-1,
	.frame_idx_mask = FrameQty-1
};

stream_t i2c1OutStream = 
{
	.data_idx_entr = 0,
	.data_idx_sal = 0,
	.data_idx_sal_aux = 0,
	.data_count_entr = 0,
	.data_count_sal = 0,
	.data_count_sal_aux = 0,
	.frame_idx_entr = 0,
	.frame_idx_sal = 0,
	.frame_count = 0,
	.frame_fifo 	= i2cOutFrameBuffer,
	.data_fifo 	= i2cOutDataBuffer,
	.frame_indentifier = i2c_frame_indentifier,
	.data_idx_mask = MAX_I2C_OUT-1,
	.frame_idx_mask = FrameQty-1
};


char i2crfi_estado = I2C_ESTADO_IDLE;
char i2crfi_estado_rcv = I2C_ESTADO_RCV_IDLE;
char i2crfi_indice = 0; // to send the frame head
uint8_t stopflag = 0;

const streamIn_t  streamIn_I2C = {getI2cStreamIn,countI2cStreamIn};
const streamOut_t streamOut_I2C = {setI2cStreamOut,getAvailable_out_count}; //Count not tested

/*==================[internal functions implementations]======================*/

void Init_I2C_Driver(void)
{
	Init_Master_I2C1(400,'M');
}

uint16_t getAvailable_out_count(void) //Not Tested
{
    return MAX_I2C_OUT - ( (MAX_I2C_OUT-1) & (i2c1OutStream.data_idx_sal - i2c1OutStream.data_idx_entr ) );
}

uint16_t countI2cStreamIn(void)
{
	return i2c1InStream.data_count_sal;
}

char getI2cStreamIn(void)
{
	return pop_I2C_IN();
}

void setI2cStreamOut(char data)
{
	push_I2C_OUT(data);
}

void I2C_Begin_Transmision(void)
{
	if (!IsStartI2c1())
	{
		{
			streamOpenReadFrame(&i2c1OutStream);
			i2crfi_indice = -2;// pongo el indice en -2 para mandar los 2 bytes del id del modulo
			i2crfi_estado = I2C_ESTADO_SENDING;
			Start_I2C1();
		}
	}
}

void ini_I2C_IN(void)
{
	i2c1InStream.data_idx_entr 		= 0;
	i2c1InStream.data_idx_sal 		= 0;
	i2c1InStream.data_idx_sal_aux 	= 0;
	i2c1InStream.data_count_entr 	= 0;
	i2c1InStream.data_count_sal 		= 0;
	i2c1InStream.data_count_sal_aux 	= 0;
	i2c1InStream.frame_idx_entr 		= 0;
	i2c1InStream.frame_idx_sal 		= 0;
	i2c1InStream.frame_count		 	= 0;
}

void ini_I2C_OUT(void)
{
	i2c1OutStream.data_idx_entr 		= 0;
	i2c1OutStream.data_idx_sal 		= 0;
	i2c1OutStream.data_idx_sal_aux 	= 0;
	i2c1OutStream.data_count_entr 	= 0;
	i2c1OutStream.data_count_sal 	= 0;
	i2c1OutStream.data_count_sal_aux = 0;
	i2c1OutStream.frame_idx_entr 	= 0;
	i2c1OutStream.frame_idx_sal 		= 0;
	i2c1OutStream.frame_count 		= 0;
}

void ini_I2C_OUT_FM(void)
{
	i2c1OutStream.frame_count = 0;
}

void reset_I2C_FRAME(void)
{
	i2c1OutStream.data_idx_sal = i2c1OutStream.data_idx_sal_aux;
    i2c1OutStream.data_count_sal = i2c1OutStream.data_count_sal_aux; 
}

void push_I2C_IN(char dato)
{
	if (dato == i2c_frame_indentifier && i2c1InStream.data_count_entr)
	{
		i2cCloseWriteFrame(&i2c1InStream);
		return;
	}
	i2c1InStream.data_fifo[i2c1InStream.data_idx_entr] = dato;
    i2c1InStream.data_idx_entr ++;
    i2c1InStream.data_count_entr ++;
    i2c1InStream.data_idx_entr &= i2c1InStream.data_idx_mask;
}

void push_I2C_OUT(char dato)
{
	i2c1OutStream.data_fifo[i2c1OutStream.data_idx_entr] = dato;
    i2c1OutStream.data_idx_entr ++;
    i2c1OutStream.data_count_entr ++;
    i2c1OutStream.data_idx_entr &= i2c1OutStream.data_idx_mask;
	if (dato == i2c_frame_indentifier)
	{
		streamOpenWriteFrame(&i2c1OutStream);
	}
}

char pop_I2C_IN(void)
{
	unsigned char dato;
    dato = i2c1InStream.data_fifo[i2c1InStream.data_idx_sal];
    i2c1InStream.data_idx_sal ++;
    i2c1InStream.data_count_sal --;
    i2c1InStream.data_idx_sal &= i2c1InStream.data_idx_mask;
    return dato;
}

char pop_I2C_OUT(void)
{
    unsigned char dato;
    dato = i2c1OutStream.data_fifo[i2c1OutStream.data_idx_sal];
    i2c1OutStream.data_idx_sal ++;
    i2c1OutStream.data_count_sal --;
    i2c1OutStream.data_idx_sal &= i2c1OutStream.data_idx_mask;
    return dato;
}

void pop_I2C_OUT_FM(void)
{
	while(i2c1OutStream.data_count_sal)
	{
		pop_I2C_OUT();
	}	
}

void i2cCloseWriteFrame(stream_t* stream)
{
        stream->frame_fifo[stream->frame_idx_entr] = stream->data_count_entr;
        stream->frame_idx_entr ++;
        stream->frame_count ++;
        stream->data_count_entr = 0;
        stream->frame_idx_entr &= stream->frame_idx_mask;
}

void i2cOpenReadFrame(stream_t* stream)
{
    stream->data_count_sal = stream->frame_fifo[stream->frame_idx_sal];
    stream->data_idx_sal_aux = stream->data_idx_sal;
    stream->data_count_sal_aux = stream->data_count_sal;
    stream->frame_idx_sal ++;
    stream->frame_count --;
    stream->frame_idx_sal &= stream->frame_idx_mask;
}

inline void I2c_driver_callback_slave(void)
{
	uint8_t d;
	if (IsReceiveBufferFullI2c1())
	{
		d = Read_I2C1(0);

		if (!IsDataOrAddressI2c1())
		{
			i2crfi_estado_rcv = I2C_ESTADO_RCV_ID; //I2C_ESTADO_PKT_ID;
			return;
		}
		else if (i2crfi_estado_rcv == I2C_ESTADO_RCV_ID)
		{
			if ( d != I2C_ID )
			{
				i2crfi_estado_rcv = I2C_ESTADO_RCV_DATA;
			}
			else
			{	
				i2crfi_estado_rcv = I2C_ESTADO_RCV_IDLE;
			}
		}
		else if (i2crfi_estado_rcv == I2C_ESTADO_RCV_DATA)
		{
			push_I2C_IN(d);
		}	
	}	
}


inline void I2c_driver_callback_master(void)
{
	if (IsCollisionDetectI2c1())
	{
		CollisionReset_I2C1();
		i2crfi_estado = I2C_ESTADO_IDLE;
		reset_I2C_FRAME();
		return;
	}	

	if (IsStopI2c1())
	{
		i2crfi_estado= I2C_ESTADO_IDLE;
		return;
	}
	if (IsStartI2c1())
	{
		if (i2crfi_estado == I2C_ESTADO_SENDING)
		{
			if (i2crfi_indice == -2) //-3)
			{
				Write_I2C1(0);
				i2crfi_indice++;
			}	
			else if (i2crfi_indice == -1)
			{
				Write_I2C1(I2C_ID);
				i2crfi_indice++;
			}	
			else
			{
				char d;
				d =  pop_I2C_OUT();
				if (d == i2c_frame_indentifier)
				{
					i2crfi_estado = I2C_ESTADO_END;
				}
				Write_I2C1(d);
			}				
		}
		else if (i2crfi_estado == I2C_ESTADO_END)
		{
			i2crfi_estado = I2C_ESTADO_IDLE;
			stopflag=1;
			Stop_I2C1();
		}
	}		
}	

/*==================[end of file]============================================*/
#endif
