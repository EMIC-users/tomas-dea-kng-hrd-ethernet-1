
#ifndef PIC_timer_H_
#define PIC_timer_H_

/*==================[inclusions]=============================================*/

#include <xc.h>

/*==================[external data declaration]==============================*/

typedef void (*Timer4_Callbak_t)(void);

void timer4_init (Timer4_Callbak_t);

/*==================[end of file]============================================*/
#endif

