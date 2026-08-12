#ifndef ENC28J60_DRIVER_H_
#define ENC28J60_DRIVER_H_

#include <stdint.h>

void     enc28j60_init(const uint8_t *mac);            // reset HW + MAC/PHY + RX on
uint8_t  enc28j60_revid(void);                         // EREVID (B7 = 0x06)
int      enc28j60_link_up(void);                       // PHSTAT2.LSTAT
int      enc28j60_send(const uint8_t *frame, uint16_t len);   // 1 = ok
uint16_t enc28j60_recv(uint8_t *buf, uint16_t buflen); // bytes leídos; 0 = nada

#endif

