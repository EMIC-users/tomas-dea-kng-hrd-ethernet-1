
/*==================[inclusions]=============================================*/
#include <xc.h>
#include "inc/system.h"
#include <libpic30.h>
#include "inc/gpio.h"
#include "inc/spi1.h"
#include "inc/enc28j60.h"

/*==================[macros]=================================================*/
// Opcodes SPI del ENC28J60
#define OP_RCR 0x00     // read control register
#define OP_WCR 0x40     // write control register
#define OP_BFS 0x80     // bit field set
#define OP_BFC 0xA0     // bit field clear
#define OP_RBM 0x3A     // read buffer memory
#define OP_WBM 0x7A     // write buffer memory

// Registros presentes en todos los bancos
#define ESTAT  0x1D
#define ECON2  0x1E
#define ECON1  0x1F
#define EIR    0x1C

// Banco 0 — punteros de buffer
#define ERDPTL   0x00
#define ERDPTH   0x01
#define EWRPTL   0x02
#define EWRPTH   0x03
#define ETXSTL   0x04
#define ETXSTH   0x05
#define ETXNDL   0x06
#define ETXNDH   0x07
#define ERXSTL   0x08
#define ERXSTH   0x09
#define ERXNDL   0x0A
#define ERXNDH   0x0B
#define ERXRDPTL 0x0C
#define ERXRDPTH 0x0D

// Banco 1
#define EPKTCNT  0x19

// Banco 2 — MAC / MII
#define MACON1   0x00
#define MACON3   0x02
#define MACON4   0x03
#define MABBIPG  0x04
#define MAIPGL   0x06
#define MAIPGH   0x07
#define MAMXFLL  0x0A
#define MAMXFLH  0x0B
#define MICMD    0x12
#define MIREGADR 0x14
#define MIWRL    0x16
#define MIWRH    0x17
#define MIRDL    0x18
#define MIRDH    0x19

// Banco 3 — dirección MAC / MII status / revisión
#define MAADR5   0x00
#define MAADR6   0x01
#define MAADR3   0x02
#define MAADR4   0x03
#define MAADR1   0x04
#define MAADR2   0x05
#define MISTAT   0x0A
#define EREVID   0x12

// Registros PHY
#define PHCON1   0x00
#define PHCON2   0x10
#define PHSTAT2  0x11

// Layout de la SRAM de 8 KB: RX 0x0000-0x19FF (6,5 KB), TX el resto
#define RXST 0x0000
#define RXND 0x19FF
#define TXST 0x1A00

#define ENC_CS_LOW()   HAL_GPIO_PinSet(ETH_CS, GPIO_LOW)
#define ENC_CS_HIGH()  HAL_GPIO_PinSet(ETH_CS, GPIO_HIGH)

/*==================[internal data]==========================================*/
static uint16_t next_pk = RXST;

/*==================[internal functions]=====================================*/
static uint8_t rcr(uint8_t addr)
{
    uint8_t v;
    ENC_CS_LOW();
    xchangeSPI1b_8(OP_RCR | (addr & 0x1F));
    v = xchangeSPI1b_8(0x00);
    ENC_CS_HIGH();
    return v;
}

// Los registros MAC/MII devuelven un byte dummy antes del dato
static uint8_t rcr_mac(uint8_t addr)
{
    uint8_t v;
    ENC_CS_LOW();
    xchangeSPI1b_8(OP_RCR | (addr & 0x1F));
    xchangeSPI1b_8(0x00);
    v = xchangeSPI1b_8(0x00);
    ENC_CS_HIGH();
    return v;
}

static void wcr(uint8_t addr, uint8_t v)
{
    ENC_CS_LOW();
    xchangeSPI1b_8(OP_WCR | (addr & 0x1F));
    xchangeSPI1b_8(v);
    ENC_CS_HIGH();
}

static void bf(uint8_t op, uint8_t addr, uint8_t mask)
{
    ENC_CS_LOW();
    xchangeSPI1b_8(op | (addr & 0x1F));
    xchangeSPI1b_8(mask);
    ENC_CS_HIGH();
}

static void set_bank(uint8_t bank)
{
    bf(OP_BFC, ECON1, 0x03);
    if (bank & 0x03)
        bf(OP_BFS, ECON1, bank & 0x03);
}

static uint16_t phy_read(uint8_t reg)
{
    uint16_t v;
    uint16_t i;

    set_bank(2);
    wcr(MIREGADR, reg);
    wcr(MICMD, 0x01);               // MIIRD
    __delay_us(11);                 // >= 10.24 us
    set_bank(3);
    for (i = 0; i < 1000; i++) {    // BUSY, acotado: chip mudo no cuelga el loop
        if (!(rcr_mac(MISTAT) & 0x01))
            break;
        __delay_us(10);
    }
    set_bank(2);
    wcr(MICMD, 0x00);
    v  = rcr_mac(MIRDL);
    v |= (uint16_t)rcr_mac(MIRDH) << 8;
    return v;
}

static void phy_write(uint8_t reg, uint16_t v)
{
    uint16_t i;

    set_bank(2);
    wcr(MIREGADR, reg);
    wcr(MIWRL, v & 0xFF);
    wcr(MIWRH, v >> 8);
    __delay_us(11);
    set_bank(3);
    for (i = 0; i < 1000; i++) {    // BUSY, acotado: chip mudo no cuelga el loop
        if (!(rcr_mac(MISTAT) & 0x01))
            break;
        __delay_us(10);
    }
}

/*==================[external functions]=====================================*/
void enc28j60_init(const uint8_t *mac)
{
    uint16_t i;

    HAL_GPIO_PinCfg(ETH_CS, GPIO_OUTPUT);
    ENC_CS_HIGH();
    HAL_GPIO_PinCfg(ETH_RST, GPIO_OUTPUT);
    HAL_GPIO_PinSet(ETH_RST, GPIO_HIGH);

    SPI1_init(0, 0);     // 8 bits, modo 0,0; default FCY/2 = 8 MHz

    // Reset por hardware y espera del oscilador (ESTAT.CLKRDY)
    HAL_GPIO_PinSet(ETH_RST, GPIO_LOW);
    __delay_ms(2);
    HAL_GPIO_PinSet(ETH_RST, GPIO_HIGH);
    __delay_ms(2);
    for (i = 0; i < 1000; i++) {
        if (rcr(ESTAT) & 0x01)
            break;
        __delay_us(100);
    }

    // Buffer RX (el hardware recibe ahí; TX usa el resto de la SRAM)
    set_bank(0);
    wcr(ERXSTL, RXST & 0xFF);
    wcr(ERXSTH, RXST >> 8);
    wcr(ERXNDL, RXND & 0xFF);
    wcr(ERXNDH, RXND >> 8);
    wcr(ERXRDPTL, RXST & 0xFF);
    wcr(ERXRDPTH, RXST >> 8);

    // MAC: habilitar RX, padding+CRC, frame length check, half duplex
    set_bank(2);
    wcr(MACON1, 0x0D);          // MARXEN | TXPAUS | RXPAUS
    wcr(MACON3, 0x32);          // PADCFG0 | TXCRCEN | FRMLNEN
    wcr(MACON4, 0x40);          // DEFER
    wcr(MAMXFLL, 1518 & 0xFF);
    wcr(MAMXFLH, 1518 >> 8);
    wcr(MABBIPG, 0x12);
    wcr(MAIPGL, 0x12);
    wcr(MAIPGH, 0x0C);

    set_bank(3);
    wcr(MAADR1, mac[0]);
    wcr(MAADR2, mac[1]);
    wcr(MAADR3, mac[2]);
    wcr(MAADR4, mac[3]);
    wcr(MAADR5, mac[4]);
    wcr(MAADR6, mac[5]);

    // PHY: half duplex, sin loopback interno
    phy_write(PHCON1, 0x0000);
    phy_write(PHCON2, 0x0100);  // HDLDIS

    // Habilitar recepción
    next_pk = RXST;
    bf(OP_BFS, ECON1, 0x04);    // RXEN
}

uint8_t enc28j60_revid(void)
{
    set_bank(3);
    return rcr(EREVID);
}

int enc28j60_link_up(void)
{
    return (phy_read(PHSTAT2) >> 10) & 1;   // LSTAT
}

int enc28j60_send(const uint8_t *frame, uint16_t len)
{
    uint16_t end = TXST + len;  // TXST = byte de control; datos hasta TXST+len
    uint16_t k;

    // Errata B7 (TX logic puede colgarse): resetear la lógica TX por paquete
    bf(OP_BFS, ECON1, 0x80);    // TXRST
    bf(OP_BFC, ECON1, 0x80);
    bf(OP_BFC, EIR, 0x0A);      // TXERIF | TXIF

    set_bank(0);
    wcr(ETXSTL, TXST & 0xFF);
    wcr(ETXSTH, TXST >> 8);
    wcr(EWRPTL, TXST & 0xFF);
    wcr(EWRPTH, TXST >> 8);

    ENC_CS_LOW();
    xchangeSPI1b_8(OP_WBM);
    xchangeSPI1b_8(0x00);    // byte de control: usar config de MACON3
    for (k = 0; k < len; k++)
        xchangeSPI1b_8(frame[k]);
    ENC_CS_HIGH();

    wcr(ETXNDL, end & 0xFF);
    wcr(ETXNDH, end >> 8);

    bf(OP_BFS, ECON1, 0x08);    // TXRTS: disparar transmisión
    for (k = 0; k < 10000; k++) {
        if (!(rcr(ECON1) & 0x08))
            break;
        __delay_us(10);
    }
    if (rcr(ECON1) & 0x08) {    // no terminó: abortar
        bf(OP_BFC, ECON1, 0x08);
        return 0;
    }
    return !(rcr(ESTAT) & 0x02);    // TXABRT
}

uint16_t enc28j60_recv(uint8_t *buf, uint16_t buflen)
{
    uint8_t hdr[6];
    uint16_t len, erxrdpt, k;

    set_bank(1);
    if (rcr(EPKTCNT) == 0)
        return 0;

    set_bank(0);
    wcr(ERDPTL, next_pk & 0xFF);    // posicionarse en el paquete actual
    wcr(ERDPTH, next_pk >> 8);

    ENC_CS_LOW();
    xchangeSPI1b_8(OP_RBM);
    for (k = 0; k < 6; k++)         // header: next ptr (2) + status (4)
        hdr[k] = xchangeSPI1b_8(0x00);
    next_pk = hdr[0] | ((uint16_t)hdr[1] << 8);
    len     = hdr[2] | ((uint16_t)hdr[3] << 8);

    if (!(hdr[4] & 0x80))           // status "Received Ok"
        len = 0;
    else if (len >= 4)
        len -= 4;                   // descartar CRC
    if (len > buflen)
        len = buflen;
    for (k = 0; k < len; k++)
        buf[k] = xchangeSPI1b_8(0x00);
    ENC_CS_HIGH();

    // Liberar espacio. Errata B7: ERXRDPT debe quedar en dirección impar
    erxrdpt = (next_pk == RXST) ? RXND : next_pk - 1;
    wcr(ERXRDPTL, erxrdpt & 0xFF);
    wcr(ERXRDPTH, erxrdpt >> 8);
    bf(OP_BFS, ECON2, 0x40);        // PKTDEC

    return len;
}
/*==================[end of file]============================================*/

