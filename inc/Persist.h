#ifndef __PERSIST_H__
#define __PERSIST_H__

#include <xc.h>
#define FOSC 32000000
#define FCY (FOSC/2)



void save(void);

void load(void);

#endif //__PERSIST_H__

