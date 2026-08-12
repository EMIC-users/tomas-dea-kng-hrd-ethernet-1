 #include <xc.h>
 #include "inc/EMICBus.h"
 #include "inc/I2C1_driver.h"
 #include "inc/I2C1.h"
 #include "inc/streamIn.h"
 #include "inc/stream.h"
 #include "inc/userFncFile.h"

#ifndef i2c_frame_indentifier
#define i2c_frame_indentifier 0
#endif

 enum
{
	tipoTrama_noUsar=0,
	tipoTrama_mensaje=1,
	tipoTrama_ejecutable=2,
	tipoTrama_control=3,
	tipoTrama_bootloader=4,
	tipoTrama_buffer=5
};

void pI2C(char* format_,...)
{
	va_list arg;
    va_start(arg, format_);
	push_I2C_OUT(tipoTrama_mensaje);

	sendDataToStream(&streamOut_I2C,format_,arg);

	va_end(arg);
	push_I2C_OUT(i2c_frame_indentifier);
}

void pI2C_ctrl(char* format_,...)
{
	va_list arg;
    va_start(arg, format_);
	push_I2C_OUT(tipoTrama_control);

	sendDataToStream(&streamOut_I2C,format_,arg);

	va_end(arg);
	push_I2C_OUT(i2c_frame_indentifier);
}

void pI2C_boot(char* format_,...)
{
	va_list arg;
    va_start(arg, format_);
	push_I2C_OUT(tipoTrama_bootloader);
	push_I2C_OUT('\t');

	sendDataToStream(&streamOut_I2C,format_,arg);

	va_end(arg);
	push_I2C_OUT(i2c_frame_indentifier);
}

void EMICBus_init()
{
	Init_I2C_Driver();
	ini_I2C_IN();				//not needed?
	ini_I2C_OUT();				//not needed?
	ini_I2C_OUT_FM();			//not needed?
}

void poll_EMICBus(void)
{
	char d;
	if(IsReceiveOverflowI2c1())//si hay un overflow lo resetea
	{
		OverflowReset_I2C1();
	}

	if (i2c1InStream.frame_count)
	{
		streamOpenReadFrame(&i2c1InStream);
		d=pop_I2C_IN();
		if (d == 0 && i2c1InStream.data_count_sal > 0)
		{
			d = pop_I2C_IN();
		}
		switch(d)
		{
			case tipoTrama_mensaje:;
			while (i2c1InStream.data_count_sal)
			{
				pop_I2C_IN();
			}
				break;
			case tipoTrama_control:;
			if (i2c1InStream.data_count_sal > 0)
			{
				switch(pop_I2C_IN())
				{
					case 'R'://si es un reset
						//reset_cpu();
						break;
					default:
						while (i2c1InStream.data_count_sal)
							pop_I2C_IN();
					break;
				}
			}
				break;
			default:
				while (i2c1InStream.data_count_sal)
					pop_I2C_IN();
				break;
		}	
	}
	
	if(stopflag==1 && IsStopI2c1() )
	{
		stopflag=0;
		pop_I2C_OUT_FM();
	}
	if (i2c1OutStream.frame_count && (!IsStartI2c1()))
	{
		if(stopflag == 1)
		{
			stopflag=0;
			//pop_I2C_OUT_FM();
		}
		else
		{
			I2C_Begin_Transmision();
		}
	}
}
