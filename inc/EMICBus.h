#ifndef _EMICBus_H_
#define _EMICBus_H_

void EMICBus_init(void);

void pI2C(char* format,...);
void pI2C_ctrl(char* format,...);
void pI2C_boot(char* format,...);

void poll_EMICBus(void);

#endif
